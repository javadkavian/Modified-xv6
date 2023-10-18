#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int compare_char(char a, char b){
  int first, second;
  if('a' <= a && a <= 'z')
    first = a - 'a';
  else
    first = a - 'A';
  if('a' <= b && b <= 'z')
    second = b - 'a';
  else
    second = b - 'A';

  if(first >= second)
    return 0;
  return 1;
}


int
main(int argc, char *argv[])
{
  if(argc != 3){
    printf(2, "Error: number of args...\n");
    exit();
  }

  char ans[16];

  int i;
  for(i = 0; argv[1][i] != '\0' && argv[2][i] != '\0'; i++)
    ans[i] = '0' + compare_char(argv[1][i], argv[2][i]);

  int j = i;
  if(argv[1][i] == '\0' && argv[2][i] != '\0')
    for(j = i; argv[2][j] != '\0'; j++)
      ans[j] = '1';
  else if(argv[1][i] != '\0' && argv[2][i] == '\0')
    for(j = i; argv[1][j] != '\0'; j++)
      ans[j] = '0';

  ans[j] = '\n';
  ans[j + 1] = '\0';

  unlink("strdiff_result.txt");
  int fd = open("strdiff_result.txt", O_CREATE | O_RDWR);
  write(fd, ans, strlen(ans));

  close(fd);
  exit();
}