#ifndef _SERVERDETAIL_H_
#define _SERVERDETAIL_H_

#include <windows.h>
#include <winsock2.h>
#include"public_detail.h"

#pragma comment(lib, "ws2_32.lib")
// #define BUFFER_LEN 1024
#define MAX_CLIENT_NUM 32
// #define SERVER_PORT 8888
// #define IP_BUF_SIZE 129
// #define NAME_LEN 64
void init_server();

/**
 * 服务端
 * 
*/
class Server {
 public:
  Server();
  ~Server();
  void WaitClient();

 private:
  WORD winsock_ver;             //winsock版本
  WSADATA wsa_data;             //winsock初始化信息
  SOCKET sock_svr;              //服务端SOCKET
  SOCKET sock_clt;              //客户端
  HANDLE h_thread;              //线程
  SOCKADDR_IN addr_svr;         //服务端ip地址信息
  SOCKADDR_IN addr_clt;         //客户端ip地址信息
  int ret_val;                  //初始化winsock的返回状态
  int addr_len;                 //ip地址长度
  char buf_ip[IP_BUF_SIZE];     //缓冲
};

#endif