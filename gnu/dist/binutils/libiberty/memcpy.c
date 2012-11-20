/* memcpy (the standard C function)
   This function is in the public domain.  */

/*

@deftypefn Supplemental void* memcpy (void *@var{out}, const void *@var{in}, size_t @var{length})

Copies @var{length} bytes from memory region @var{in} to region
@var{out}.  Returns a pointer to @var{out}.

@end deftypefn

*/

#include <ansidecl.h>
#ifdef ANSI_PROTOTYPES
#include <stddef.h>
#else
#define size_t unsigned long
#endif

void bcopy PARAMS((const void*, void*, size_t));

PTR
memcpy (out, in, length)
     PTR out;
     const PTR in;
     size_t length;
{
    bcopy(in, out, length);
    return out;
}
