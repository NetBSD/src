#	$NetBSD: Makefile,v 1.13 1999/03/01 04:05:18 simonb Exp $
#	@(#)Makefile	8.2 (Berkeley) 2/16/94

S=	${.CURDIR}/../../../..

LIB=	sa

# from sys/arch/pmax/stand/libsa:
SRCS=	alloc.c callvec.c devopen.c memcpy.c memset.c ufs.c
# PROM callbacks
SRCS+=	bootinit.S bootread.S clear_cache.S printf.S strcat.S strcmp.S \
	strcpy.S strlen.S
# from sys/lib/libsa:
SRCS+=	disklabel.c dkcksum.c open.c read.c twiddle.c

.PATH:	$S/lib/libsa

# set up for standalone library compiation (turn off PIC)
MKPROFILE=no
MKPIC=	no
MKLINT=	no

# only needed during build - prevent installation of library
libinstall::

.include <bsd.lib.mk>
.include "../Makefile.inc"	# XXX after to reset CFLAGS, etc
