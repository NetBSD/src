#	$NetBSD: Makefile,v 1.8 1999/01/21 14:16:52 simonb Exp $
#	@(#)Makefile	8.2 (Berkeley) 2/16/94

S=	${.CURDIR}/../../../..

LIB=	sa

# XXX: Reordering SRCS changes the library order and the resultant output file
# XXX: Check that things work with this order (not just compile).
## from sys/arch/pmax/stand/libsa:
#SRCS=	alloc.c callvec.c devopen.c getenv.c gets.c read.c strcat.c \
#	strcmp.c strcpy.c ufs.c
## from sys/lib/libsa:
#SRCS+=	bcopy.c bzero.c close.c dev.c disklabel.c dkcksum.c getfile.c \
#	ioctl.c lseek.c open.c write.c

SRCS=	alloc.c bcopy.c bzero.c close.c dev.c dkcksum.c disklabel.c \
	getfile.c ioctl.c lseek.c open.c read.c ufs.c write.c \
	callvec.c devopen.c getenv.c gets.c strcat.c strcmp.c strcpy.c


.PATH:	$S/lib/libsa $S/lib/libkern

# set up for standalone library compiation (turn off PIC)
NOPROFILE=noprofile
NOPIC	=nopic
NOLINT	=nolint

# only needed during build - prevent installation of library
libinstall::

.include <bsd.lib.mk>
.include "../Makefile.inc"	# XXX after to reset CFLAGS, etc
