#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>

int main(void){
pid_t pid;

switch (pid = fork()){
  case 0:
    printf("child\n");
    break;
  default:
    printf("parent\n");
    break;
  }
  return 0;
}
