#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"
#include "x86.h"

char*
strcpy(char *s, const char *t)
{
  char *os;

  os = s;
  while((*s++ = *t++) != 0)
    ;
  return os;
}

int
strcmp(const char *p, const char *q)
{
  while(*p && *p == *q)
    p++, q++;
  return (uchar)*p - (uchar)*q;
}

uint
strlen(const char *s)
{
  int n;

  for(n = 0; s[n]; n++)
    ;
  return n;
}

void*
memset(void *dst, int c, uint n)
{
  stosb(dst, c, n);
  return dst;
}

char*
strchr(const char *s, char c)
{
  for(; *s; s++)
    if(*s == c)
      return (char*)s;
  return 0;
}

char*
gets(char *buf, int max)
{
  int i, cc;
  char c;

  for (i = 0; i + 1 < max - 1;)
  {
    cc = read(0, &c, 1);
    if (cc < 1)
      break;
    if (c == '\n' || c == '\r')
      break;
 
    if (c == 2)
      i--;
    else if(c == 6)
      i++;    
    else if (c== 127 || c == 8)
    {
      if (i> 0)
      {
        
        for (int x = i ; x<max-1 ; x++ )
        {
          
          buf[x-1]=buf[x];
        }
        i--;
        buf[max-1]='\0';
      }
    }
    else
    {
      if (buf[i] == 0)
      {
        buf[i] = c;
      }
      else
      {
        for (int j = max - 1; j >= i; j--)
        {
          buf[j + 1] = buf[j];
        }
        buf[i] = c;
      }
      i++;
    }
  }
  // buf[i] = '\0';
  return buf;
}

int
stat(const char *n, struct stat *st)
{
  int fd;
  int r;

  fd = open(n, O_RDONLY);
  if(fd < 0)
    return -1;
  r = fstat(fd, st);
  close(fd);
  return r;
}

int
atoi(const char *s)
{
  int n;

  n = 0;
  while('0' <= *s && *s <= '9')
    n = n*10 + *s++ - '0';
  return n;
}

void*
memmove(void *vdst, const void *vsrc, int n)
{
  char *dst;
  const char *src;

  dst = vdst;
  src = vsrc;
  while(n-- > 0)
    *dst++ = *src++;
  return vdst;
}