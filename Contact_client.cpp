#include <windows.h>
#include <winsock2.h>

#include <iostream>
#include <limits>

#include "client_detail.h"

using namespace std;

char name[NAME_LEN + 1];

DWORD rec_threa;

void *RecvMessage(LPVOID data) {
  SOCKET cli_detail =  (SOCKET) data;

  // message buffer
  string message_buffer;
  int message_len = 0;

  // one transfer buffer
  char buffer[BUFFER_LEN + 1];
  int buffer_len = 0;

  // receive
  while ((buffer_len = recv(cli_detail, buffer, BUFFER_LEN, 0)) > 0) {
    // to find '\n' as the end of the message
    for (int i = 0; i < buffer_len; i++) {
      if (message_len == 0)
        message_buffer = buffer[i];
      else
        message_buffer += buffer[i];
      message_len++;
      if (buffer[i] == '\n') {
        // print out the message
        cout << message_buffer << endl;

        // new message start
        message_len = 0;
        message_buffer.clear();
      }
    }
    memset(buffer, 0, sizeof(buffer));
  }
  cout << "The Server has been shutdown!\n";
  return NULL;
}
void DealSend(SOCKET server_sock) {
  while (1) {
    char message[BUFFER_LEN + 1];
    cin.get(message, BUFFER_LEN);
    int n = strlen(message);
    if (cin.eof()) {
      cin.clear();
      clearerr(stdin);
      continue;
    }
    //只输入回车
    else if (n == 0) {
      cin.clear();
      clearerr(stdin);
    }
    if (n > BUFFER_LEN - 2) {
      cin.clear();
      cin.ignore(numeric_limits<streamsize>::max(), '\n');
      cout << "消息内容过长" << endl;
      continue;
    }
    cin.get();  //移除 \n
    message[n] = '\n';
    message[n + 1] = '\0';

    n++;
    cout << endl;
    int sent_len = 0;

    int trans_len = BUFFER_LEN > n ? n : BUFFER_LEN;

    while (n > 0) {
      int len = ::send(server_sock, message + sent_len, trans_len, 0);
      if (len < 0) {
        perror("消息发送错误");
        // return -1;
      }
      n -= len;
      sent_len += len;
      trans_len = BUFFER_LEN > n ? n : BUFFER_LEN;
    }
    memset(message, 0, sizeof(message));
  }
  ::CloseHandle(&::rec_threa);
  shutdown(server_sock, 2);
}

Client::Client() {
  //构造函数
  cout << "客户端启动中" << endl;
  winsock_version = MAKEWORD(2, 2);
  addr_len = sizeof(SOCKADDR_IN);
  cli_addr.sin_family = AF_INET;
  cli_addr.sin_addr.S_un.S_addr = inet_addr(SERVER_IP);
  cli_addr.sin_port = htons(SERVER_PORT);
  memset(buf_ip, 0, IP_BUF_SIZE);

  retval = ::WSAStartup(winsock_version, &wsa_data);
  if (retval != 0) {
    cerr << "WSA启动失败,错误代码" << ::WSAGetLastError() << endl;
    system("pause");
    exit(1);
  }

  cli_socket = ::socket(AF_INET, SOCK_STREAM, 0);
  if (cli_socket == INVALID_SOCKET) {
    perror("socket创建失败");
    exit(1);
  }

  if (::connect(cli_socket, (struct sockaddr *)&cli_addr, sizeof(cli_addr)) !=
      0) {
    cout<<WSAGetLastError()<<endl;
    perror("连接服务端失败");
    exit(1);
  }

  cout << "Connecting=====>>>>>" << endl;
  fflush(stdout);
  char state[10] = {0};
  if (recv(cli_socket, state, sizeof(state), 0) < 0) {
    perror("接收失败");
    exit(1);
  }
  if (strcmp(state, "OK")) {
    //接收到服务端的非OK消息
    cout << "聊天室已经满员" << endl;
    exit(1);
  } else {
    cout << "====加入聊天室成功====" << endl;
  }
  /// get name ////
  cout << "欢迎使用多人聊天室!" << endl;
  while (1) {
    cout << "请输入你的昵称:";
    cin.get(::name, NAME_LEN);
    int name_len = strlen(name);
    // no input
    if (cin.eof()) {
      cin.clear();
      clearerr(stdin);
      cout << "请输入至少一个字符" << endl;
      continue;
    } else if (name_len == 0) {
      cin.clear();
      clearerr(stdin);
      cin.get();
      continue;
      cout << "请输入至少一个字符" << endl;
    } else if (name_len > NAME_LEN - 2) {
      cin.clear();
      cin.ignore(numeric_limits<streamsize>::max(), '\n');
      cout << "昵称过长" << endl;
      continue;
    }
    cin.get();
    name[name_len] = '\0';
    break;
  }
  if (send(cli_socket, ::name, strlen(::name), 0) < 0) {
    perror("发送昵称错误");
    exit(1);
  }
  cout << "昵称确认成功!" << endl;
  ::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)RecvMessage,
                 (void *)cli_socket, 0, &::rec_threa);
  ::DealSend(cli_socket);
}

Client::~Client() {
  ::closesocket(cli_socket);
  ::WSACleanup();
  cout << "客户端关闭" << endl;
}

int main(void) {
  Client client = Client();
  return 0;
}