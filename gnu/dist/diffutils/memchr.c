/* Support memchr on systems where it doesn't work.  */

#include <config.h>
#include <sys/types.h>

void *
memchr (s, c, n)
     void const *s;
     int c;
     size_t n;
{
  unsigned char const *p = s;
  unsigned char const *lim = p + n;

  for (;  p < lim;  p++)
    if (*p == c)
      return p;
  return 0;
}
