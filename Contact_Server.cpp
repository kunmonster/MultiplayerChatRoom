#include <server_detail.h>
#include <windns.h>
#include <winsock2.h>

#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

#pragma comment(lib, "ws2_32.lib")
using namespace std;

typedef struct _CLIENT_ {
  int status;                   // current status
  SOCKET socket;                // current user socket
  char username[NAME_LEN + 1];  // user name
  int userId;                   // user id
} Client;
Client client[MAX_CLIENT_NUM] = {0};

queue<string> message_q[MAX_CLIENT_NUM];

int current_client_num = 0;

// thread
DWORD send_thread[MAX_CLIENT_NUM];
DWORD chat_thread[MAX_CLIENT_NUM];

std::mutex current_client_num_mtx;  //用户数量互斥量
std::mutex mtx;                     //定义一个mutex对象,互斥量
std::condition_variable cv;
HANDLE client_mutex[MAX_CLIENT_NUM] = {0};  //用户数组每个用户的互斥量

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

DWORD WINAPI ChattClientThrea(LPVOID client_info);

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
        // ::unique_lock<std::mutex> lck(current_client_num_mtx);
        ::current_client_num_mtx.lock();
        //加锁
        memset(client[i].username, 0, NAME_LEN);
        strcpy(client[i].username, client_name);
        client[i].userId = i;
        client[i].status = 1;
        client[i].socket = client_socket;
        ::client_mutex[i] = ::CreateMutex(
            NULL, FALSE, NULL);  //用户信息填充完毕,为该用户创建互斥量
        ::current_client_num++;
        ::current_client_num_mtx.unlock();
        CreateThread(NULL, 0, ChattClientThrea, (void*)&client[i], 0,
                     &::chat_thread[i]);
        std::cout << client[i].username << " "
                  << "join in the chat room. Online User Number:"
                  << current_client_num << endl;
        break;
      }
    }
  }
  for (int i = 0; i < MAX_CLIENT_NUM; i++)
    if (client[i].status) shutdown(client[i].socket, 2);
  shutdown(sock_svr, 2);
}
/**
 * 消息发送函数
 *
 */
DWORD WINAPI SendThrea(LPVOID client_info) {
  Client* client_detail = (struct _CLIENT_*)client_info;
  while (true) {
    WaitForSingleObject(&::client_mutex[client_detail->userId], INFINITE);
    while (::message_q[client_detail->userId].empty()) {
      //当消息队列为空
    }
    //发送消息
    while (!::message_q[client_detail->userId].empty()) {
      string message_buffer = message_q[client_detail->userId].front();
      int size = message_buffer.length();
      int trans_len = BUFFER_LEN > size ? size : BUFFER_LEN;
      while (size > 0) {
        int retlen =
            ::send(client_detail->socket, message_buffer.c_str(), trans_len, 0);
        if (retlen < 0) {
          perror("send");
        }
        size -= retlen;
        message_buffer.erase(0, retlen);
        trans_len = BUFFER_LEN > size ? size : BUFFER_LEN;
      }
      message_buffer.clear();
      message_q[client_detail->userId].pop();
    }
    //释放互斥量
    ReleaseMutex(&::client_mutex[client_detail->userId]);
  }
}

/**
 * 消息接收线程
 */
DWORD WINAPI RecvMessage(LPVOID client_info) {
  Client* client_detail = (struct _CLIENT_*)client_info;

  // message
  string message_buffer;
  int message_len = 0;

  // buffer
  char buffer[BUFFER_LEN + 1];
  int buffer_len = 0;

  // recvive message
  while ((buffer_len = recv(client_detail->socket, buffer, BUFFER_LEN, 0)) >
         0) {
    //接收到消息
    for (int i = 0; i < buffer_len; i++) {
      // the start of a new message
      if (message_len == 0) {
        char temp[100];
        sprintf(temp, "s%:", client_detail->username);
        message_buffer = temp;
        message_len = message_buffer.length();
      }
      message_buffer += buffer[i];
      message_len++;
      if (buffer[i] == '\n') {
        // sned to every client

        for (int j = 0; j < MAX_CLIENT_NUM; j++) {
          if (::client[j].status &&
              ::client[j].socket != client_detail->socket) {
            WaitForSingleObject(&::client_mutex[j], INFINITE);
            ::message_q[j].push(message_buffer);
            //信号量
            ::ReleaseMutex(&::client_mutex[j]);
          }
        }
        message_buffer.clear();
        message_len = 0;
      }
    }
    ::memset(buffer,0,sizeof(buffer));
  }
}

/**
 * 聊天线程处理
 */
DWORD ChatClientThrea(LPVOID client_info) {
  ::Client* client_detail = (struct _CLIENT_*)client_info;

  // send welcome message
  char welcome[100];
  sprintf(welcome,
          "Hello %s, Welcome to join the chat room. Online User Number: %d\n",
          client_detail->username, current_client_num);
  ::WaitForSingleObject(&client_mutex[client_detail->userId], INFINITE);
  ::message_q->push(welcome);
  //
  ::ReleaseMutex(&client_mutex[client_detail->userId]);
  memset(welcome, 0, sizeof(welcome));
  sprintf(welcome, "New User %s join in! Online User Number: %d\n",
          client_detail->username, current_client_num);
  // send messages to other users
  for (int j = 0; j < MAX_CLIENT_NUM; j++) {
    if (::client[j].status && client[j].socket != client_detail->socket) {
      ::WaitForSingleObject(&client_mutex[j], INFINITE);
      message_q[j].push(welcome);
      // pthread_cond_signal(&cv[j]);
      ::ReleaseMutex(&client_mutex[j]);
    }
  }
  //创建发送线程
  CreateThread(NULL, 0, SendThrea, (LPVOID)&client_detail, 0,
               &::send_thread[client_detail->userId]);
}

int main(void) { return 0; }
