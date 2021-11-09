#ifndef _CLIENT_DETAIL_H_
#define _CLIENT_DETAIL_H_

#include<winsock2.h>
#include<windows.h>
#include"public_detail.h"

#pragma comment(lib,"ws2_32.lib")



class Client{
  public:
    Client();
    ~Client();
  private:
    WORD winsock_version;
    WSADATA wsa_data;
    SOCKET svr_socket;
    SOCKET cli_socket;
    SOCKADDR_IN cli_addr;
    // SOCKADDR_IN svr_addr;
    int addr_len;
    int retval;
    char buf_ip[IP_BUF_SIZE];     //缓冲
};

#endif