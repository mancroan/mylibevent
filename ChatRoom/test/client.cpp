#include <iostream>
#include <string>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <fstream>

using namespace std;

#define SEND_BUF_SIZE 1024
#define RECV_BUF_SIZE 1024
#define MAX_CONNECT_NUM 1

void *recv_ser(void *arg);
void *input_msg(void *arg);
void client_socket();
void input_chat_name();
void input_target_name();
void get_ip_local();
void get_ip_config();

char        addrs_buf[15];
char        buf_recv[RECV_BUF_SIZE];
char        buf_send[SEND_BUF_SIZE];
bool        flag_time(false);
int         sClient[MAX_CONNECT_NUM];
int         sleep_sec(4);
void        *tret;
time_t      start,end;
double      dif(2);
pthread_t   tid[2] = {0};
pthread_mutex_t     mutex;
pthread_attr_t      attr;
struct  sockaddr_in ser;
char*        test_name;

int main(int argc, char* argv[])
{
    if(argc > 1)
    {
        sleep_sec = atoi(argv[1]);
        if(argc > 2)
        {
            test_name = argv[2];
        }
    }
 
    pthread_mutex_init(&mutex,NULL);
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);  
    memset(buf_recv, 0, sizeof(buf_recv));
    memset(buf_send, 0, sizeof(buf_send));
    get_ip_local();
    client_socket();	
    //input_chat_name();
    while(1)
    {
        pthread_create(&tid[0], &attr, recv_ser, NULL);
        pthread_create(&tid[1], &attr, input_msg, NULL);
        while(1)
        {

        };
    }
    cout << "disconnect with server" << endl;

    return 0;
}
// socketͨ��
void client_socket()
{
    ser.sin_family = AF_INET;
    ser.sin_port = htons(6666);
    inet_pton(AF_INET, addrs_buf, &ser.sin_addr);
    for(int i = 0; i < MAX_CONNECT_NUM; i++)
    {
        sClient[i] = socket(AF_INET, SOCK_STREAM, 0);	
        connect(sClient[i], (struct sockaddr *)&ser, sizeof(ser));   
        send(sClient[i], test_name, sizeof(test_name),0);     // ���Զ�����ѹ�� 
    }
}
// ����client�û���,���͵�������
void input_chat_name()
{
    char name[10];

    cout << "input name" << endl;
    while(cin >> name)
    {
        if(strlen(name) < 10)
        {
            break;
        }
        else 
        {
            cout << "name too long,input again" << endl;
            continue;
        }
    }
    send(sClient[0], name, sizeof(name),0); 
}
// �߳�1:���ܷ�����������Ϣ
void *recv_ser(void *arg)
{
    int recv_err = recv(sClient[0], buf_recv, sizeof(buf_recv), 0);   
    if(recv_err > 0)
    {
        pthread_mutex_lock(&mutex);
        if(buf_recv[0] != '\0')
        {
            printf("recv data from %s\n", buf_recv);
        }
        buf_recv[0] = '\0'; 
        pthread_mutex_unlock(&mutex);
        pthread_create(&tid[0], &attr, recv_ser, NULL);
    }
    else
    {
        cout << "disconnect with server" << endl;
        close(sClient[0]);
        exit(0);
    }
    pthread_exit((void *)0);
}
// �߳�2:����������Ϣ
void *input_msg(void *arg)
{
    /*
    while(cin >> buf_send)
    {
        if(flag_time)
        {
            time(&start);
            flag_time = false;
        }
        else
        {
            time(&end);
            flag_time = true;
        }
        dif = difftime(end, start);
        if(dif > -2 && dif < 2)                 // ��ֹƵ������
        {
            printf("input too fast! wait\n");
            break;
        }
        else if(strlen(buf_send) >20)           // ��ֹ��������
        {
            printf("message too long! in 20\n");
            continue;
        }
        else
        {
            send(sClient[0], buf_send, sizeof(buf_send), 0);
            break;
        }
    }
    */
    sleep(sleep_sec);
    send(sClient[0], "all:123", sizeof("all:123"), 0);
    pthread_create(&tid[1], &attr, input_msg, NULL);
    pthread_exit((void *)1);
}
// ������ȡ����IP
void get_ip_local() 
{
    struct ifaddrs * ifAddrStruct=NULL;
    void * tmpAddrPtr=NULL;

    getifaddrs(&ifAddrStruct);

    while (ifAddrStruct != NULL)
    {
        if (ifAddrStruct->ifa_addr->sa_family == AF_INET)
        {
            if(strcmp(ifAddrStruct->ifa_name, "lo"))
            {
                tmpAddrPtr=&((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
                inet_ntop(AF_INET, tmpAddrPtr, addrs_buf, INET_ADDRSTRLEN);
                printf("%s IP Address %s\n", ifAddrStruct->ifa_name, addrs_buf);
            }
        }
        ifAddrStruct=ifAddrStruct->ifa_next;
    }
}
// �������ļ���ȡip
void get_ip_config()
{
    ifstream fin("config.txt");  
    fin.getline(addrs_buf, 15);
    cout << "Read from file: " << addrs_buf << endl; 
}
