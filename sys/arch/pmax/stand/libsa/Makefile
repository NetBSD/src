#	$NetBSD: Makefile,v 1.11 1999/02/13 02:54:48 lukem Exp $
#	@(#)Makefile	8.2 (Berkeley) 2/16/94

S=	${.CURDIR}/../../../..

LIB=	sa

# from sys/arch/pmax/stand/libsa:
SRCS=	alloc.c callvec.c devopen.c memcpy.c read.c ufs.c
# from sys/lib/libsa:
SRCS+=	bzero.c disklabel.c dkcksum.c open.c

.PATH:	$S/lib/libsa $S/lib/libkern

# set up for standalone library compiation (turn off PIC)
MKPROFILE=no
MKPIC=	no
MKLINT=	no

# only needed during build - prevent installation of library
libinstall::

.include <bsd.lib.mk>
.include "../Makefile.inc"	# XXX after to reset CFLAGS, etc
