///////////////////////////////////////////////////////
// 基于poll的tc_server
///////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>

typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;
typedef struct pollfd pollfd;

void Init(pollfd* fd_list, int size)
{
  int i;
  for(i = 0; i < size; ++i)
  {
    fd_list[i].fd = -1;
    fd_list[i].events = 0;
    fd_list[i].revents = 0;
  }
}

void Add(int fd, pollfd* fd_list, int size)
{
  int i;
  for(i = 0; i < size; ++i)
  {
    if(fd_list[i].fd == -1)
    {
      fd_list[i].fd = fd;
      fd_list[i].events = POLLIN;
      break;
    }
  }
}

int main(int argc, char* argv[]) {
  if(argc != 3)
  {
    printf("Usag: ./poll_server [ip] [port]");
    return 1;
  }
  
  struct sockaddr_in addr;
  addr.sin_family      = AF_INET;
  addr.sin_addr.s_addr = inet_addr(argv[1]);
  addr.sin_port        = htons(atoi(argv[2]));

  int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if(listen_fd < 0)
  {
    perror("socket");
    return 1;
  }
  int ret = bind(listen_fd, (sockaddr*)&addr, sizeof(addr));
  if(ret < 0)
  {
    perror("bind");
    return 1;
  }
  ret = listen(listen_fd, 5);
  if(ret < 0)
  {
    perror("listen");
    return 1;
  }
  pollfd fd_list[1024];
  Init(fd_list, sizeof(fd_list)/sizeof(fd_list[0]));
  Add(listen_fd, fd_list, sizeof(fd_list)/sizeof(pollfd));
  while(1)
  {
    int ret = poll(fd_list,sizeof(fd_list)/sizeof(pollfd), 1000);
    if(ret < 0)
    {
      perror("pollk");
      continue;
    }
    if(ret == 0)
    {
      printf("poll timeout\n");
      continue;
    }
    size_t i;
    for(i = 0; i < sizeof(fd_list)/sizeof(pollfd); ++i)
    {
      if(fd_list[i].fd == -1)
      {
        continue;
      }
      if(!(fd_list[i].revents & POLLIN))
      {
        continue;
      }
      if(fd_list[i].fd == listen_fd)
      {
        //处理 listen_fd的情况
        struct sockaddr_in clinet_addr;
        socklen_t len = sizeof(clinet_addr);
        int connect_fd = accept(listen_fd, (sockaddr*)&clinet_addr, &len);
        if(connect_fd < 0)
        {
          perror("accept");
          continue;
        }
        Add(connect_fd, fd_list, sizeof(fd_list)/sizeof(pollfd));
      }
      else
      {
        //处理connect_fd的情况
        char buf[1024] = {0};
        ssize_t read_size = read(fd_list[i].fd, buf, sizeof(buf));
        if(read_size < 0)
        {
          perror("read");
          continue;
        }
        if(read_size == 0)
        {
          printf("clinet disconnect\n");
          close(fd_list[i].fd);
          fd_list[i].fd = -1;
        }
        printf(("client say: %s\n"), buf);
        write(fd_list[i].fd, buf, strlen(buf));
      }
    }
  }
  return 0;
}

