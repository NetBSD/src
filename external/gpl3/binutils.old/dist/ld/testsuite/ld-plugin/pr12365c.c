extern void abort (void);
extern int inside_main;
typedef __SIZE_TYPE__ size_t;

#define TEST_ABORT if (inside_main) abort()

void *
my_memcpy (void *d, const void *s, size_t n)
{
  char *dst = (char *) d;
  const char *src = (const char *) s;
  while (n--)
    *dst++ = *src++;
  return (char *) d;
}

void
my_bcopy (const void *s, void *d, size_t n)
{
  char *dst = (char *) d;
  const char *src = (const char *) s;
  if (src >= dst)
    while (n--)
      *dst++ = *src++;
  else
    {
      dst += n;
      src += n;
      while (n--)
        *--dst = *--src;
    }
}

void *
my_memset (void *d, int c, size_t n)
{
  char *dst = (char *) d;
  while (n--)
    *dst++ = c;
  return (char *) d;
}

void
my_bzero (void *d, size_t n)
{
  char *dst = (char *) d;
  while (n--)
    *dst++ = '\0';
}

void *
memcpy (void *d, const void *s, size_t n)
{
  void *result = my_memcpy (d, s, n);
  TEST_ABORT;
  return result;
}

void
bcopy (const void *s, void *d, size_t n)
{
  my_bcopy (s, d, n);
  TEST_ABORT;
}

void *
memset (void *d, int c, size_t n)
{
  void *result = my_memset (d, c, n);
  TEST_ABORT;
  return result;
}

void
bzero (void *d, size_t n)
{
  my_bzero (d, n);
  TEST_ABORT;
}
