#	$NetBSD: Makefile.inc,v 1.111.2.9 2002/03/22 20:42:34 nathanw Exp $
#	@(#)Makefile.inc	8.3 (Berkeley) 10/24/94

# Generation and compilation of syscall stubs have moved to
# libsyscall. Conversion and compatibility interfaces are here.

# sys sources
.PATH: ${.CURDIR}/sys

# glue to provide compatibility between GCC 1.X and 2.X and for compat
# with old syscall interfaces.
SRCS+=	adjtime.c clock_settime.c ftruncate.c getdirentries.c lseek.c mmap.c \
	msync.c ntp_adjtime.c pread.c preadv.c pwrite.c pwritev.c semctl.c \
	settimeofday.c sigaltstack.c stat.c swapon.c truncate.c

# glue to syscalls that may pass structs or unions, which cannot be
# handled portably and easily by the syscall interface. The glue
# converts structs and unions to pointers to them.
SRCS+=	__semctl13.c
