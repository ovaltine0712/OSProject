#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

//0 - read 1 - write
int
main(int argc, char *argv[])
{
  int p1[2], p2[2];
  int ppid, cpid;
  char buf[1];
  pipe(p1);
  pipe(p2);

  //child
  if(fork() == 0){
    //start
    close(p1[1]);
    close(p2[0]);

    cpid = getpid();
    read(p1[0], buf, 1);
    fprintf(1,"%d: received ping\n",cpid);

    write(p2[1],"x",1);
    //finished
    close(p1[0]);
    close(p2[1]);

  }else {
    close(p1[0]);
    close(p2[1]);

    ppid = getpid();
    write(p1[1],"x",1);

    read(p2[0],buf,1);
    fprintf(1,"%d: received pong\n",ppid);

    close(p1[1]);
    close(p2[0]);
  }
  exit(0);  
}

