#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void create_child_process();
//0 - read 1 - write
int
main(int argc, char *argv[])
{
  int p1[2];
  pipe(p1);

  if(fork() != 0){
    close(p1[0]);
    for(int i = 2; i <= 35; i++){
    	write(p1[1], &i, sizeof(int));
    }
    close(p1[1]);
    wait(0);
  }else{
    create_child_process(p1);
  }

  exit(0);
}


void create_child_process(int p[]){
  int cp[2];
  int x,y;
  close(p[1]);

  if(read(p[0],&x,sizeof(int))){
    fprintf(1, "prime %d\n", x);
    pipe(cp);
    if(fork() != 0){
      close(cp[0]);
      while(read(p[0],&y,sizeof(int))){
        if(y%x != 0){
	  write(cp[1], &y, sizeof(int));
	}
     }
	 close(p[0]);
	 close(cp[1]);
	 wait(0);
    }else{
     create_child_process(cp);
   }
  }

   exit(0);
}
