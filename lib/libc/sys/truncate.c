/* $Id: truncate.c,v 1.1 1994/04/02 05:38:22 cgd Exp $ */

#include <sys/types.h>
#include <sys/syscall.h>

int
truncate(path, length)
        char    *path;
        off_t   length;
{

        return(__syscall((quad_t)SYS_truncate, path, 0, length));
}
