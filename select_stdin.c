///////////////////////////////////////////////////////
// 循环从标准输入读取数据， 并写到标准输出
// 使用select完成文件描述符的等待过程
///////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>

int main()
{
  //1.先把文件描述符集准备好
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(0, &fds);
  while(1)
  {
    printf(">");
    fflush(stdout);
    int ret = select(1, &fds, NULL, NULL, NULL);
    if(ret < 0)
    {
      perror("select");
      continue;
    }
    //如果select 返回，并且不是错误返回，就只可能是0号文件描述符读就绪了
    if(FD_ISSET(0, &fds))
    {
      char buf[1024] = {0};
      read(0, buf, sizeof(buf) - 1);
      printf("input:%s\n", buf);
    }
    else{
      printf("error! invaild fd\n");
      continue;
    }
    FD_ZERO(&fds);
    FD_SET(0, &fds);
  }

  return 0;
}
