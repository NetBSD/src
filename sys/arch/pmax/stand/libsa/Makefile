#	$NetBSD: Makefile,v 1.10 1999/01/30 12:23:10 simonb Exp $
#	@(#)Makefile	8.2 (Berkeley) 2/16/94

S=	${.CURDIR}/../../../..

LIB=	sa

# from sys/arch/pmax/stand/libsa:
SRCS=	alloc.c callvec.c devopen.c memcpy.c read.c ufs.c
# from sys/lib/libsa:
SRCS+=	bzero.c disklabel.c dkcksum.c open.c

.PATH:	$S/lib/libsa $S/lib/libkern

# set up for standalone library compiation (turn off PIC)
NOPROFILE=noprofile
NOPIC	 =nopic
NOLINT	 =nolint

# only needed during build - prevent installation of library
libinstall::

.include <bsd.lib.mk>
.include "../Makefile.inc"	# XXX after to reset CFLAGS, etc
