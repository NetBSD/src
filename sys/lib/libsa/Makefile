#	$Id: Makefile,v 1.1 1994/01/26 02:03:32 brezak Exp $

LIB=	sa

CFLAGS+=-DSTANDALONE -I${.CURDIR}

# io routines
SRCS+=	close.c dev.c disklabel.c ioctl.c lseek.c open.c read.c \
	stat.c ufs.c write.c

# other misc routines
SRCS+=	alloc.c bcopy.c exit.c getfile.c gets.c printf.c strerror.c

NOPROFILE=
NOPIC=
OBJMACHINE=

install:

.include <bsd.lib.mk>
