#include <iostream>
#include <fcntl.h>
#include "socket_manager.h"
#include "global.h"

using namespace std;

void sock_info_t::init()
{
	m_serv_addr.sin_family = AF_INET;
	m_serv_addr.sin_port = htons(6666);
	m_serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	m_listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    int flags;
    //fcntl()用来操作文件描述符的一些特性
    if ((flags = fcntl(m_listen_fd, F_GETFL)) == -1) {
        return;
    }

    if (fcntl(m_listen_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        return;
    }

	int opt = 1;
	setsockopt(m_listen_fd,SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if(bind(m_listen_fd, (struct sockaddr*)&m_serv_addr, sizeof(m_serv_addr)) == -1)
    {
        cout << "bind error" << endl;
        close(m_listen_fd);
    }
	if(listen(m_listen_fd, 5) == -1)
    {
        cout << "listen error" << endl;
        close(m_listen_fd);
    }
	m_len = sizeof(m_cli_addr);
	cout << "server init succeed" << endl;
}
