/* $Id: ftruncate.c,v 1.1 1994/04/02 05:38:16 cgd Exp $ */

#include <sys/types.h>
#include <sys/syscall.h>

int
ftruncate(path, length)
        char    *path;
        off_t   length;
{

        return(__syscall((quad_t)SYS_ftruncate, path, 0, length));
}
