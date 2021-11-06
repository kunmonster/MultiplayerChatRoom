#include <server_detail.h>
#include <windns.h>
#include <winsock2.h>

#include <iostream>
#include <mutex>
#include <thread>

#pragma comment(lib, "ws2_32.lib")
using namespace std;

struct _CLIENT_ {
  int status;                   // current status
  SOCKET socket;                // current user socket
  char username[NAME_LEN + 1];  // user name
  int userId;                   // user id
} client[MAX_CLIENT_NUM] = {0};

int current_client_num = 0;

std::mutex mtx;  //定义一个mutex对象,互斥量

HANDLE client_mutex[MAX_CLIENT_NUM] = {0};        //每个用户的互斥量


HANDLE mutex = CreateMutex(NULL,FALSE,NULL);


Server::Server() {
  cout << "服务端启动中" << endl;
  winsock_ver = MAKEWORD(2, 2);
  addr_len = sizeof(SOCKADDR_IN);
  addr_svr.sin_family = AF_INET;
  addr_svr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
  addr_svr.sin_port = htons(SERVER_PORT);
  memset(buf_ip, 0, IP_BUF_SIZE);
  //
  ret_val = ::WSAStartup(winsock_ver, &wsa_data);
  if (ret_val != 0) {
    cerr << "WSA failed to start up! Error code" << ::WSAGetLastError() << endl;
    system("pause");
    exit(1);
  }
  //
  sock_svr = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock_svr == INVALID_SOCKET) {
    cerr << "Failed to create server socket!Error code: " << ::WSAGetLastError()
         << endl;
    ::WSACleanup();
    system("pause");
    exit(1);
  }
  cout << "Server socket is created successfully!" << endl;

  //
  if ((ret_val = ::bind(sock_svr, (SOCKADDR*)&addr_svr, addr_len)) != 0) {
    //绑定失败
    cerr << "Failed to bind server socket!Error code: " << ::WSAGetLastError()
         << endl;
    ::WSACleanup();
    system("pause");
    exit(1);
  }
  cout << "Server socket is bound successfully...\n";
  //
  if ((ret_val = ::listen(sock_svr, 20)) == SOCKET_ERROR) {
    cerr << "Server socket failed to listen!Error code: " << ::WSAGetLastError()
         << "\n";
    ::WSACleanup();
    system("pause");
    exit(1);
  }
  cout << "Server socket started to listen...\n";
  //
  cout << "服务端启动成功!" << endl;
}

Server::~Server() {
  ::closesocket(sock_svr);
  ::closesocket(sock_clt);
  ::WSACleanup();
  cout << "服务端关闭" << endl;
}

DWORD WINAPI creatClientThrea(LPVOID client_info);

void Server::WaitClient() {
  int sum = 0;
  while (true) {
    //等待客户端连接然后创建新线程处理客户端
    SOCKET client_socket = ::accept(sock_svr, NULL, NULL);
    if (client_socket == -1) {
      perror("accept");
      exit(1);
    }
    if (::current_client_num >= MAX_CLIENT_NUM) {
      //连接数量太多
      if (send(client_socket, "ERROR", strlen("ERROR"), 0) < 0) {
        perror("send");
        ::shutdown(client_socket, 2);
        continue;
      }
    } else {
      if (send(client_socket, "OK", strlen("OK"), 0) < 0) {
        perror("ok");
      }
    }
    //与客户端连接成功

    //获取用户名
    char client_name[NAME_LEN + 1] = {0};
    int state = recv(client_socket, client_name, NAME_LEN, 0);
    if (state < 0) {
      perror("recv");
      shutdown(client_socket, 2);
      continue;
    } else if (state == 0) {
      shutdown(client_socket, 2);
      continue;
    }

    for (int i = 0; i < MAX_CLIENT_NUM; i++) {
      if (!client[i].status) {
        //找到空的位置,然后将该用户放进去
        ::mtx.lock();
        //加锁
        memset(client[i].username, 0, NAME_LEN);
        strcpy(client[i].username, client_name);
        client[i].userId = i;
        client[i].status = 1;
        client[i].socket = client_socket;
        ::client_mutex[i] =  ::CreateMutex(NULL,FALSE,NULL);    //用户信息填充完毕,为该用户创建互斥量


        
        ::current_client_num++;
        ::mtx.unlock();
      }
    }
  }
}

DWORD creatClientThrea(LPVOID client_info) {}

int main(void) { return 0; }

void init_server() {
  //初始化DLL
  WSADATA wsaData;
  int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (err != 0) perror("DLL初始化错误");

  //创建套接字
  SOCKET servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

  //绑定套接字
  sockaddr_in sockAddr;
  memset(&sockAddr, 0, sizeof(sockAddr));
  sockAddr.sin_family = PF_INET;
  sockAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");  // ip地址
  sockAddr.sin_port = htons(SERVER_PORT);
  bind(servSock, (SOCKADDR*)&sockAddr, sizeof(SOCKADDR));
  //进入监听状态
  listen(servSock, 20);
}