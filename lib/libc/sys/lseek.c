/* $Id: lseek.c,v 1.1 1994/04/02 05:38:18 cgd Exp $ */

#include <sys/types.h>
#include <sys/syscall.h>

off_t
lseek(fd, offset, whence)
        int     fd;
        off_t   offset;
        int     whence;
{
        extern off_t __syscall();

        return(__syscall((quad_t)SYS_lseek, fd, 0, offset, whence));
}
