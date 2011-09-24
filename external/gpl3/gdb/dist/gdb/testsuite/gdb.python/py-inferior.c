#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define CHUNK_SIZE 16000 /* same as findcmd.c's */
#define BUF_SIZE (2 * CHUNK_SIZE) /* at least two chunks */

int8_t int8_search_buf[100];
int16_t int16_search_buf[100];
int32_t int32_search_buf[100];
int64_t int64_search_buf[100];

static char *search_buf;
static int search_buf_size;


int f2 (int a)
{
  char *str = "hello, testsuite";

  puts (str);	/* Break here.  */

  return ++a;
}

int f1 (int a, int b)
{
  return f2(a) + b;
}

static void
init_bufs ()
{
  search_buf_size = BUF_SIZE;
  search_buf = malloc (search_buf_size);
  if (search_buf == NULL)
    exit (1);
  memset (search_buf, 'x', search_buf_size);
}

int main (int argc, char *argv[])
{
  init_bufs ();

  return f1 (1, 2);
}
