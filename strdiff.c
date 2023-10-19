#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"


void make_lower_case(char* A){
  int i = 0;
  while(A[i] != '\0'){
    if(!(A[i] > 'a' && A[i] < 'z')){
      A[i] = A[i] + 32;
    }
    i++;
  }
}


int
main(int argc, char *argv[])
{
  if(argc != 3){
    printf(2, "Not enough arguments\n");
    exit();
  }
  make_lower_case(argv[1]);
  make_lower_case(argv[2]);
  int A_size = strlen(argv[1]);
  int B_size = strlen(argv[2]);
  char ans[15];
  int iterator = 0;
  while(iterator != A_size && iterator != B_size){
    if(argv[1][iterator] >= argv[2][iterator]){
      ans[iterator] = '0';
    }
    else{
      ans[iterator] = '1';
    }
    iterator++;
  }
  if(iterator == A_size){
    while(iterator != B_size){
      ans[iterator] = '1';
      iterator ++;
    }
  }
  else if(iterator == B_size){
    while(iterator != A_size){
      ans[iterator] = '0';
      iterator ++;
    }
  }
  ans[iterator] = '\n';
  ans[iterator+1] = '\0';

  int fd = open("strdiff_result.txt", O_CREATE | O_RDWR);
  write(fd, ans, strlen(ans));
  close(fd);
  exit();
}