///////////////////////////////////////////////////////
// 回显服务器,基于select实现
///////////////////////////////////////////////////////
//select中的nfds表示的含义是后面三个位图结构中保存的最大的文件描述符+1

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;

typedef struct FdSet{
  fd_set set;
  int max_fd;
}FdSet;

void FdSetInit(FdSet* fds)
{
  fds->max_fd = -1;
  FD_ZERO(&fds->set);
}

void FdSetAdd(FdSet* fds, int fd)
{
  FD_SET(fd, &fds->set);
  if(fd > fds->max_fd)
  {
    fds->max_fd = fd;
  }
}

void FdSetDel(FdSet* fds, int fd)
{
  FD_CLR(fd, &fds->set);
  int max_fd = -1;
  int i = 0;
  for(; i < fds->max_fd; ++i)
  {
    if(!FD_ISSET(i, &fds->set))
    {
      continue;
    }
    if(i > max_fd)
    {
     max_fd = i; 
    }
  }

  //此时max_fd 值就已经是最新的文件描述符集中的最大值了。
  fds->max_fd = max_fd;
  
}
///////////////////////////////////////////////////////
// select.server
///////////////////////////////////////////////////////

int ServerInit(const char* ip, short port)
{
  int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
  if(listen_sock < 0)
  {
    perror("socket");
    return -1;
  }
  
  sockaddr_in addr;
  addr.sin_family      = AF_INET;
  addr.sin_addr.s_addr = inet_addr(ip);
  addr.sin_port        = htons(port);
  int ret = bind(listen_sock, (sockaddr*)&addr, sizeof(addr));
  if(ret < 0)
  {
    perror("bind");
    return -1;
  }
  
  ret = listen(listen_sock, 5);
  if(ret < 0)
  {
    perror("listen");
    return -1;
  }
  return listen_sock;

}

int ProcessRequest(int new_sock)
{
  char buf[1024] = {0};
  ssize_t read_size = read(new_sock, buf, sizeof(buf) - 1);
  if(read_size < 0)
  {
    perror("read");
    return -1;
  }
  if(read_size == 0)
  {
    printf("[client%d] disconnected\n", new_sock);
    close(new_sock);
    return 0;
  }
  buf[read_size] = '\0';
  printf("[client%d]: %s\n", new_sock, buf);
  write(new_sock, buf, strlen(buf));
   return 1; 
}


int main(int argc, char* argv[]) {
  if(argc != 3)
  {
    printf("Usage: ./server [ip] [port]\n");
    return 1;
  }
  int listen_sock = ServerInit(argv[1], atoi(argv[2]));
  if(listen_sock < 0)
  {
    printf("ServerInit failed\n");
    return 1;
  }
  printf("ServerInit ok\n");
  FdSet fds;
  FdSetInit(&fds);
  FdSetAdd(&fds, listen_sock);
  while(1)
  {
    ///////////////////////////////////////////////////////
    // 此时我们要使用 select 完成所有文件描述符就绪的等待
    // 按照下面的代码来实现, 其实还有一个致命缺陷.
    // 例如: 调用 select 的时候 fds.set 包含了100个文件描述符
    // 说明需要关注100个文件描述符的就绪状态.
    // 当 select 返回的时候, 假设此时只有1个文件描述符就绪.
    // fds.set 也就只包含1位为1了
    // 
    // 此时, 如果循环结束, 第二次再调用 select, fds.set里面就只剩一位了
    ///////////////////////////////////////////////////////
    FdSet output_fds = fds; //这里对fds进行备份
    int ret = select(fds.max_fd + 1, &output_fds.set, NULL, NULL, NULL);
    if(ret < 0)
    {
      perror("select");
      continue;
    }
    if(FD_ISSET(listen_sock, &output_fds.set))
    {
      //就绪的文件描述符是listen_sock，此时意味着有新的客户端
      //连接上了服务器
      sockaddr_in peer;
      socklen_t len = sizeof(peer);
      int new_sock = accept(listen_sock, (sockaddr*)&peer, &len);
      if(new_sock < 0)
      {
        perror("accept");
        continue;
      }
      FdSetAdd(&fds, new_sock);
      printf("[client %d] connect!\n", new_sock);
    }
    else{
      //用于遍历文件描述符集,i的含义也就是表示一个文件描述符
      int i = 0;
      for(; i <= output_fds.max_fd; ++i)
      {
        if(!FD_ISSET(i, &output_fds.set))
        {
          //当前的文件描述符不再返回的文件描述符集之中---没就绪
          continue;
        }
        //此时发现某个new_sock读就绪
        //从new_sock中读取一次请求，进行计算，并进行响应
        int ret = ProcessRequest(i);
        //此处需要判定ret如果是0， 表示对方断开连接。此时就应该吧对应的
        //socket从select的位图上进行删除
        if(ret == 0)
        {
          FdSetDel(&fds, i);
        } //end if(ret == 0)
      }//end for(; i <= fds.max_fd; ++i)
    }//end if (FD_ISSET(listen_sock, &fds.set))
  }//end while(1)


  return 0;
}


