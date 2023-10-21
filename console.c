// Console input and output.
// Input is from the keyboard or serial port.
// Output is written to the screen and serial port.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"

static void consputc(int);

static int panicked = 0;

static struct
{
  struct spinlock lock;
  int locking;
} cons;
static void
printint(int xx, int base, int sign)
{
  static char digits[] = "0123456789abcdef";
  char buf[16];
  int i;
  uint x;

  if (sign && (sign = xx < 0))
    x = -xx;
  else
    x = xx;

  i = 0;
  do
  {
    buf[i++] = digits[x % base];
  } while ((x /= base) != 0);

  if (sign)
    buf[i++] = '-';

  while (--i >= 0)
    consputc(buf[i]);
}
// PAGEBREAK: 50
// Print to the console. only understands %d, %x, %p, %s.
void cprintf(char *fmt, ...)
{
  int i, c, locking;
  uint *argp;
  char *s;

  locking = cons.locking;
  if (locking)
    acquire(&cons.lock);

  if (fmt == 0)
    panic("null fmt");

  argp = (uint *)(void *)(&fmt + 1);
  for (i = 0; (c = fmt[i] & 0xff) != 0; i++)
  {
    if (c != '%')
    {
      consputc(c);
      continue;
    }
    c = fmt[++i] & 0xff;
    if (c == 0)
      break;
    switch (c)
    {
    case 'd':
      printint(*argp++, 10, 1);
      break;
    case 'x':
    case 'p':
      printint(*argp++, 16, 0);
      break;
    case 's':
      if ((s = (char *)*argp++) == 0)
        s = "(null)";
      for (; *s; s++)
        consputc(*s);
      break;
    case '%':
      consputc('%');
      break;
    default:
      // Print unknown % sequence to draw attention.
      consputc('%');
      consputc(c);
      break;
    }
  }

  if (locking)
    release(&cons.lock);
}

void panic(char *s)
{
  int i;
  uint pcs[10];

  cli();
  cons.locking = 0;
  // use lapiccpunum so that we can call panic from mycpu()
  cprintf("lapicid %d: panic: ", lapicid());
  cprintf(s);
  cprintf("\n");
  getcallerpcs(&s, pcs);
  for (i = 0; i < 10; i++)
    cprintf(" %p", pcs[i]);
  panicked = 1; // freeze other CPU
  for (;;)
    ;
}

// PAGEBREAK: 50
#define BACKSPACE 0x100
#define CRTPORT 0x3d4
static ushort *crt = (ushort *)P2V(0xb8000); // CGA memory
int size_of_console = 0;
static ushort hist[10][128];
int prev_comand = -1;
int last_comand = -1;

int prev_size[10];
static void
cgaputc(int c)
{
  int pos;

  // Cursor position: col + 80*row.
  outb(CRTPORT, 14);
  pos = inb(CRTPORT + 1) << 8;
  outb(CRTPORT, 15);
  pos |= inb(CRTPORT + 1);

  if (c == '\n')
  {
    pos += 80 - pos % 80;
    size_of_console = 0;
  }
  else if (c == BACKSPACE)
  {
    if (size_of_console > 0)
    {
      size_of_console--;
    }
    if (pos % 80 > 2)
    {
      --pos;
      crt[pos] = ' ' | 0x0700;
      for (int i = pos; i < 80 * 25; i++)
      {
        crt[i] = crt[i + 1];
      }
    }
    else
    {
      crt[pos] = ' ' | 0x0700;
    }
  }
  else if (c == 2)
  {
    if (pos > 0)
      --pos;
  }
  else if (c == 6)
  {
    if (pos % 80 < size_of_console)
    {
      pos++;
    }
  }
  else if (c == 227)
  {
    while (pos % 80 != 1)
    {
      crt[pos] = ' ' | 0x0700;
      pos--;
    }
    pos++;
    for (int i = 0; i < prev_size[prev_comand + 1]; i++)
    {
      crt[pos] = hist[prev_comand + 1][i];
      pos++;
    }
    size_of_console = prev_size[prev_comand];
  }
  else if (c == 12)
  {
    for (int i = 0; i < pos; i++)
    {
      crt[i] = ' ' | 0x0700;
    }
    crt[0] = ('$' & 0xff) | 0x0700;
    pos = 2;
  }
  else if (c == 226)
  {
    while (pos % 80 != 1)
    {
      crt[pos] = ' ' | 0x0700;
      pos--;
    }
    pos++;
    for (int i = 0; i < prev_size[prev_comand]; i++)
    {
      crt[pos] = hist[prev_comand][i];
      pos++;
    }
    size_of_console = prev_size[prev_comand];
  }
  else
  {
    int i = 80 * 25;
    while (i > pos)
    {
      crt[i] = crt[i - 1];
      i -= 1;
    }
    size_of_console++;
    crt[pos++] = (c & 0xff) | 0x0700;
  } // black on white

  if (pos < 0 || pos > 25 * 80)
    panic("pos under/overflow");

  if ((pos / 80) >= 24)
  { // Scroll up.
    memmove(crt, crt + 80, sizeof(crt[0]) * 23 * 80);
    pos -= 80;
    memset(crt + pos, 0, sizeof(crt[0]) * (24 * 80 - pos));
  }

  outb(CRTPORT, 14);
  outb(CRTPORT + 1, pos >> 8);
  outb(CRTPORT, 15);
  outb(CRTPORT + 1, pos);
}

void consputc(int c)
{
  if (panicked)
  {
    cli();
    for (;;)
      ;
  }

  if (c == BACKSPACE)
  {
    uartputc('\b');
    uartputc(' ');
    uartputc('\b');
  }
  else
    uartputc(c);
  cgaputc(c);
}
short arrow_flag = 0;
#define INPUT_BUF 128
struct
{
  char buf[INPUT_BUF];
  uint r; // Read index
  uint w; // Write index
  uint e; // Edit index
} input;
void shift_histor()
{
  for (int i = 0; i < 9; i++)
  {
    prev_size[i] = prev_size[i + 1];
    for (int j = 0; j < 128; j++)
    {
      hist[i][j] = hist[i + 1][j];
    }
  }
  for (int i = 0; i < 128; i++)
  {
    hist[9][i] = (' ' | 0x0700);
  }
}
#define C(x) ((x) - '@') // Control-x
int comand_size = 0;
void consoleintr(int (*getc)(void))
{
  int c, doprocdump = 0;

  acquire(&cons.lock);
  while ((c = getc()) >= 0)
  {
    switch (c)
    {
    case C('P'): // Process listing.
      // procdump() locks cons.lock indirectly; invoke later
      doprocdump = 1;
      break;
    case C('U'): // Kill line.
      while (input.e != input.w &&
             input.buf[(input.e - 1) % INPUT_BUF] != '\n')
      {
        input.e--;
        consputc(BACKSPACE);
      }
      break;
    case C('B'):
      if (input.e > input.r)
      {
        consputc(C('B'));
        if (input.e - input.r < INPUT_BUF)
        {
          input.buf[input.e++ % INPUT_BUF] = c;
        }
      }
      break;
    case C('L'):
      consputc(C('L'));
      break;
    case (C('F')):

      consputc(C('F'));
      if (input.e - input.r < INPUT_BUF)
      {
        input.buf[input.e++ % INPUT_BUF] = c;
      }

      break;
    case (226):
      if (input.e - input.r < INPUT_BUF)
      {
        arrow_flag = 1;
        input.buf[input.e++ % INPUT_BUF] = 27;
        if (prev_comand != -1)
        {
          cgaputc(c);
          prev_comand--;
        }
      }
      break;
    case (227):
      input.buf[input.e++ % INPUT_BUF] = 33;
      if (prev_comand != last_comand)
      {
        prev_comand++;
        cgaputc(c);
      }
      break;
    case 127:
    case 8:
      comand_size--;
      if (input.e - input.r < INPUT_BUF)
      {
        cgaputc(BACKSPACE);
        input.buf[input.e++ % INPUT_BUF] = 127;
      }
      break;
    default:
      if (c != 0 )
      {
        c = (c == '\r') ? '\n' : c;
        input.buf[input.e++ % INPUT_BUF] = c;
        if (c != '\n' && c != 226 && c != 227)
        {
          comand_size++;
        }
        consputc(c);
        if (arrow_flag == 1 && c == '\n')
        {

          if (last_comand != 9)
          {
            last_comand++;
          }
          else
          {
            shift_histor();
          }
          int j = 0;
          for (int i = 0; i < prev_size[prev_comand + 1]; i++)
          {

            hist[last_comand][i] =hist[prev_comand + 1][j];
            j++;
          }
          int a = j;
          j = input.w % INPUT_BUF;
          j++;
          prev_size[last_comand] = prev_size[prev_comand + 1];
          for (int i = a; input.buf[j] != '\n';)
          {

            if (input.buf[j + 1] != 127 && input.buf[j + 1] != 8)
            {
              consputc(input.buf[j]);
              if (input.buf[j] == 226)
              {
                j++;
              }
              else if (input.buf[j] == C('B'))
              {
                if (i != 0)
                {
                  j++;
                  i--;
                }
              }
              else if (input.buf[j] == C('F'))
              {
                if (hist[last_comand][i] != (' ' | 0x0700))
                {
                  i++;
                  j++;
                }
              }
              else if (input.buf[j] == C('L'))
              {
              }
              else
              {
                prev_size[last_comand]++;
                for (int x = 128; x >= i; x--)
                {
                  hist[last_comand][x + 1] = hist[last_comand][x];
                }
                hist[last_comand][i] = (input.buf[j] & 0xff) | 0x0700;
                j++;
                i++;
              }
            }
            else
            {
              j += 2;
              i--;
            }
          }
          

          prev_comand = last_comand;
          arrow_flag = 0;
          input.w = input.e;
          wakeup(&input.r);
        }
        else if (c == '\n' || c == C('D') || input.e == input.r + INPUT_BUF)
        {
          if (prev_comand != 9)
          {
            prev_comand++;
          }
          if (last_comand != 9)
          {
            last_comand++;
          }
          else
          {
            shift_histor();
          }
          int j = input.r % INPUT_BUF;
          for (int i = 0; input.buf[j] != '\n';)
          {

            if (input.buf[j + 1] != 127 && input.buf[j + 1] != 8)
            {
              if (input.buf[j] == C('B'))
              {
                if (i != 0)
                {
                  j++;
                  i--;
                }
              }
              else if (input.buf[j] == C('F'))
              {
                if (hist[last_comand][i] != (' ' | 0x0700))
                {
                  i++;
                  j++;
                }
              }
              else if (input.buf[j] == C('L'))
              {
              }
              else
              {
                for (int x = 128; x >= i; x--)
                {
                  hist[last_comand][x + 1] = hist[last_comand][x];
                }
                hist[last_comand][i] = (input.buf[j] & 0xff) | 0x0700;
                j++;
                i++;
              }
            }
            else
            {
              j += 2;
              i--;
            }
          }
          prev_size[last_comand] = comand_size;
          comand_size = 0;
          input.w = input.e;
          prev_comand = last_comand;
          wakeup(&input.r);
        }
      }
      break;
    }
  }
  release(&cons.lock);
  if (doprocdump)
  {
    procdump(); // now call procdump() wo. cons.lock held
  }
}

int consoleread(struct inode *ip, char *dst, int n)
{
  uint target;
  int c;

  iunlock(ip);
  target = n;
  acquire(&cons.lock);
  while (n > 0)
  {
    while (input.r == input.w)
    {
      if (myproc()->killed)
      {
        release(&cons.lock);
        ilock(ip);
        return -1;
      }
      sleep(&input.r, &cons.lock);
    }
    c = input.buf[input.r++ % INPUT_BUF];
    if (c == C('D'))
    { // EOF
      if (n < target)
      {
        // Save ^D for next time, to make sure
        // caller gets a 0-byte result.
        input.r--;
      }
      break;
    }
    *dst++ = c;
    --n;
    if (c == '\n')
      break;
  }
  release(&cons.lock);
  ilock(ip);

  return target - n;
}

int consolewrite(struct inode *ip, char *buf, int n)
{
  int i;

  iunlock(ip);
  acquire(&cons.lock);
  for (i = 0; i < n; i++)
    consputc(buf[i] & 0xff);
  release(&cons.lock);
  ilock(ip);

  return n;
}

void consoleinit(void)
{
  initlock(&cons.lock, "console");

  devsw[CONSOLE].write = consolewrite;
  devsw[CONSOLE].read = consoleread;
  cons.locking = 1;

  ioapicenable(IRQ_KBD, 0);
}
