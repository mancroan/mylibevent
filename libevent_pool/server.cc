#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
 
#include <sys/types.h>
#include <sys/socket.h>
 
#include <event.h>
#include <queue>
#include <iostream>
 
#include <arpa/inet.h>
 
#include <memory.h>
 
const int thread_num = 25;
#define BUF_SIZE 1024
 
using namespace std;
 
typedef struct {
    pthread_t tid;
    struct event_base *base;
    struct event pipe_event;

    int read_fd;
    int write_fd;
    //queue<int> q;
    int f_connect;
    char * buffer;
}LIBEVENT_THREAD;                 //需要保存的信息结构，用于管道通信和基事件的管理
 
typedef struct {
    pthread_t tid;
    struct event_base *base;
}DISPATCHER_THREAD;

typedef struct {
    LIBEVENT_THREAD *libevent_thread;
    struct event* event_rd;
    struct event* event_wd;
}LIBEVENT_THREAD_ARGS;




 
LIBEVENT_THREAD *threads = (LIBEVENT_THREAD *) calloc(thread_num, sizeof(LIBEVENT_THREAD));
 
void on_write(int sock, short event, void* arg)
{
    if(NULL == arg){
        return;
    }

    LIBEVENT_THREAD_ARGS* args = (LIBEVENT_THREAD_ARGS*) arg;//获取传进来的参数
    write(sock, args->libevent_thread->buffer, strlen(args->libevent_thread->buffer));
    event_free(args->event_wd);
    args->event_wd = NULL;
    free(args->libevent_thread->buffer);
    args->libevent_thread->buffer = NULL;
}
 
void on_read(int sock, short event, void* arg)
{

    if(NULL == arg){
        return;
    }
    LIBEVENT_THREAD_ARGS *args = (LIBEVENT_THREAD_ARGS *) arg;//获取传进来的参数
    char* buffer = new char[BUF_SIZE];
    memset(buffer, 0, sizeof(char)*BUF_SIZE);
    int size = read(sock, buffer, BUF_SIZE);
    if( size <= 0 )
    {
        //printf("client close!\n");
        event_free(args->event_rd);
        args->event_rd = NULL;
        close(sock);
        return ;
    }
 

    //cout<<"client: "<<buffer<<endl;

    args->libevent_thread->buffer = buffer;

    struct event* write_ev = (struct event*)malloc(sizeof(struct event));
    args->event_wd = write_ev;
    event_set(write_ev, sock, EV_WRITE, on_write, args);
    event_base_set(args->libevent_thread->base, write_ev);
    event_add(write_ev, NULL);
}

static void thread_libevent_process(int fd, short which, void *arg)
{
    int ret;
    char buf[128];
    LIBEVENT_THREAD *me = (LIBEVENT_THREAD *) arg;
    int fdconnect;
    if (fd != me->read_fd) {
        printf("thread_libevent_process error : fd != me->read_fd\n");
        exit(1);
    }
 
    ret = read(fd, buf, 128);
    if (ret > 0) 
    {
        buf[ret] = '\0';
        //printf("thread %llu receive message : %s\n", (unsigned long long)me->tid, buf);
    }
 
    //cout<<"thread_libevent_process\n"<<endl;
    /*if(me->q.size()>0)
        {
            fdconnect=me->q.front();
            me->q.pop();
 
           ret = read(fd, buf, 128);
           if (ret > 0) 
            {
 
              buf[ret] = '\0';
 
              printf("thread %llu receive message : %s\n", (unsigned long long)me->tid, buf);
 
            }
        }*/
 
        /*if(me->q.size()>0)
        {
            fdconnect=me->q.front();
 
            cout<<"thread_libevent_process succeed "<<endl;
            //me->q.pop();
        }
 
        else
            return ;*/
 
    fdconnect = me->f_connect;

    LIBEVENT_THREAD_ARGS *args = (LIBEVENT_THREAD_ARGS*)malloc(sizeof(LIBEVENT_THREAD_ARGS));

    struct event* event_rd = (struct event*)malloc(sizeof(struct event));//发生读事件后，从socket中取出数据

    args->libevent_thread = me;
    args->event_rd = event_rd;

    event_set(event_rd, fdconnect, EV_READ|EV_PERSIST, on_read, args);
    event_base_set(me->base, event_rd);
    event_add(event_rd , NULL);
 
    return;
}
 
void thread_init()
{
    int ret;
    int fd[2];
    for (int i = 0; i < thread_num; i++) {
        ret = socketpair(AF_LOCAL, SOCK_STREAM, 0, fd);
        if (ret == -1) {
            perror("socketpair()");
            return  ;
        }
        threads[i].read_fd = fd[0];
        threads[i].write_fd = fd[1];


        threads[i].base = event_init();
        if (threads[i].base == NULL) {
            perror("event_init()");
            return ;
        }
        event_set(&threads[i].pipe_event, threads[i].read_fd, EV_READ | EV_PERSIST, thread_libevent_process, &threads[i]);
        event_base_set(threads[i].base, &threads[i].pipe_event);
        if (event_add(&threads[i].pipe_event, 0) == -1) {
            perror("event_add()");
            return ;
        }
        cout<<"thread_init succeed"<<endl;
    }
}

void * worker_thread(void *arg)
{
    LIBEVENT_THREAD *me = (LIBEVENT_THREAD *)arg;
    me->tid = pthread_self();

    //event_base_loop(me->base, 0);
    event_base_dispatch(me->base);//每个工作线程都在检测event链表是否有事件发生
    return NULL;
}
 
void CreatPhreadPool()
{
    for (int i = 0; i < thread_num; i++) {
        pthread_create(&threads[i].tid, NULL, worker_thread, &threads[i]);
    }
    cout<<"CreatPhreadPool"<<endl;
}

int getSocket(){
    int fd =socket( AF_INET, SOCK_STREAM, 0 );
    if(-1 == fd){
        cout<<"Error, fd is -1"<<endl;
    }
    return fd;
}

int last_thread=0;
void accept_cb(int sock, short event, void* arg)  //添加其他信息
{
    struct sockaddr_in remote_addr;
    int sin_size=sizeof(struct sockaddr_in);
    int new_fd = accept(sock,  (struct sockaddr*) &remote_addr, (socklen_t*)&sin_size);    //如果线程池已用完，怎么办呢？
    if(new_fd < 0){
        cout<<"Accept error in on_accept()"<<endl;
        return;
    }
    //cout<<"new_fd accepted is "<<new_fd<<endl;
    evutil_make_socket_nonblocking(new_fd);

    int tid = (last_thread + 1) % thread_num;        //memcached中线程负载均衡算法
    LIBEVENT_THREAD *thread = threads + tid;
    last_thread = tid;

    thread->f_connect=new_fd;
    write(thread->write_fd, " ", 1);
}

DISPATCHER_THREAD dispatcher_thread;        //用于设置主线程的结构变量
int main(int argc, char** argv)  
{
    thread_init();
    CreatPhreadPool();
    int fd_listen = getSocket();
    if(fd_listen <0){
        cout<<"Error in main(), fd<0"<<endl;
    }

    struct sockaddr_in local_addr; //服务器端网络地址结构体
    memset(&local_addr,0,sizeof(local_addr)); //数据初始化--清零
    local_addr.sin_family=AF_INET; //设置为IP通信
    local_addr.sin_addr.s_addr=inet_addr("127.0.0.1");//服务器IP地址
    local_addr.sin_port=htons(atoi("9876")); //服务器端口号

    //允许多次绑定同一个地址。要用在socket和bind之间
    evutil_make_listen_socket_reuseable(fd_listen);

    int bind_result = bind(fd_listen, (struct sockaddr*) &local_addr, sizeof(struct sockaddr));
    if(bind_result < 0){
        cout<<"Bind Error in main()"<<endl;
        return -1;
    }

    cout<<"bind_result="<<bind_result<<endl;
    listen(fd_listen, 10);
    evutil_make_socket_nonblocking(fd_listen);
    struct event_base* base = event_base_new();
    dispatcher_thread.base=base;
    dispatcher_thread.tid = pthread_self();

    struct event listen_ev;
    event_set(&listen_ev, fd_listen, EV_READ|EV_PERSIST, accept_cb, NULL);
    event_base_set(dispatcher_thread.base, &listen_ev);
    event_add(&listen_ev, NULL);
    event_base_dispatch(dispatcher_thread.base);//监听线程
    
     //------以下语句理论上是不会走到的---------------------------
    cout<<"event_base_dispatch() in main() finished"<<endl;
    event_del(&listen_ev);
    event_base_free(dispatcher_thread.base);
    cout<<"main() finished"<<endl;
}
