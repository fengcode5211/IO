///////////////////////////////////////////////////////
//epoll   ET模式
///////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

// 异常处理   ， 将错误判断封装成为宏    -----> 错误处理代码和正常的逻辑代码相互耦合
// c++异常不推荐使用    ---    1.运行时影响性能-执行流乱跳  2.影响代码的执行逻辑

typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;
typedef struct epoll_event epoll_event;

void ProcessListenSock(int epoll_fd, int listen_sock)
{
  //1. 调用accept
  sockaddr_in peer;
  socklen_t len = sizeof(peer);
  int new_sock = accept(listen_sock, (sockaddr*)&peer, &len);
  if(new_sock < 0)
  {
    perror("accept");
    return;
  }
  //2. 把new_sock加入到epoll中
  epoll_event event;
  event.events = EPOLLIN | EPOLLET;
  event.data.fd = new_sock;   //带上这里的文件描述符
  int ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_sock, &event);
  if(ret < 0)
  {
    perror("epoll_ctk ADD");
    return;
  }
   
  printf("[Client %d] connected\n", new_sock);
  return;
}

void ProcessNewSock(int epoll_fd, int new_sock)
{
  //1.读写数据
  char buf[1024] = {0};
  ssize_t read_size = read(new_sock, buf, sizeof(buf) - 1);
  if(read_size < 0)
  {
    perror("read");
    return;
  }
  if(read_size == 0)
  {
    //2.一旦读到返回值为0，对端关闭了文件描述符
    //本端也应该关闭文件描述符，还要把本端的文件描述符从epoll中清理掉
    close(new_sock);
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, new_sock, NULL);
    printf("[Cilent %d] disconnected\n", new_sock);
    return;
  }
  buf[read_size] = '\0'; 
  printf("[Client %d] say: %s\n", new_sock, buf);
  write(new_sock, buf, strlen(buf));
  return;
}

int ServerInit(const char* ip, short port)
{
  //1. 创建socket
  int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
  if(listen_sock < 0)
  {
    perror("socket");
    return -11;
  }
  //2. 绑定端口号
  sockaddr_in addr;
  addr.sin_family      = AF_INET;
  addr.sin_addr.s_addr = inet_addr(ip);
  addr.sin_port        = htons(port);
  int ret = bind(listen_sock, (sockaddr*)&addr, sizeof(addr));
  if(ret < 0)
  {
    perror("bind");
    return 0-1;
  }
  //3. 进行监听
  ret = listen(listen_sock, SOMAXCONN);   //5-10  SOMAXCONN = 128
  if(ret < 0)
  {
    perror("listen");
    return -1;
  }
  return listen_sock;
}


int main(int argc, char* argv[]) {
  if(argc != 3)
  {
    printf("Usage : ./server_epoll [ip] [port]\n");
    return 1;
  }
  //创建并初始化socket
  int listen_sock = ServerInit(argv[1], atoi(argv[2]));
  if(listen_sock < 0)
  {
    printf("ServerInit failed\n");
    return 1;
  }
  //创建并初始化epoll对象
  // 把listen_sock放到epoll对象中
  int epoll_fd = epoll_create(10);
  if(epoll_fd < 0)
  {
    perror("epoll_create");
    return 1;
  }
  
  epoll_event event;
  event.events = EPOLLIN | EPOLLET;
  event.data.fd = listen_sock;  // key：listen_sock   value : listen_sock   epoll_wait根据返回的文件描述符的类别分别处理

  i
    uuut ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_sock, &event);
  if(ret < 0)
  {
    perror("epoll_ctl");
    return -1;
  }
  //服务器初始化完成
  printf("ServerInit Ok\n");

  //服务器循环
  while(1)
  {
    epoll_event output_event[100];
    int nfds = epoll_wait(epoll_fd, output_event, 100, -1);    //-1表示永远阻塞等待，0非阻塞   
    if(nfds < 0)
    {
      perror("epoll_wait");
      continue;
    } 
  //epoll_wait 返回之后，都有那些文件描述符就绪，就写到了 output_event里面
    int i = 0;
    for(;i < nfds; ++i)
    {
      //根据就绪的文件描述符的类别分情况进行讨论
      if(listen_sock == output_event[i].data.fd)
      {
        //1>listen_sock就绪   调用accept
        ProcessListenSock(epoll_fd, listen_sock);
      }
      else
      {
        //new_sock就绪    调用一次 read
        ProcessNewSock(epoll_fd, output_event[i].data.fd);
      }//endif
    }//end for
  }//endwhile

  return 0;
}


