/* $Id: mmap.c,v 1.1 1994/04/02 05:38:19 cgd Exp $ */

#include <sys/types.h>
#include <sys/syscall.h>

caddr_t
mmap(addr, len, prot, flags, fd, offset)
        caddr_t addr;
        size_t  len;
        int     prot;
        int     flags;
        int     fd;
        off_t   offset;
{

        return((caddr_t)__syscall((quad_t)SYS_mmap, addr, len, prot, flags,
                fd, 0, offset));
}
