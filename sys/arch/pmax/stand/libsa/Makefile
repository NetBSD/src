#	$NetBSD: Makefile,v 1.6 1999/01/21 12:33:40 simonb Exp $
#	@(#)Makefile	8.2 (Berkeley) 2/16/94

S=	${.CURDIR}/../../../..

LIB=	sa
SRCS=	alloc.c bcopy.c bzero.c close.c dev.c disklabel.c getfile.c \
	ioctl.c lseek.c open.c read.c ufs.c write.c \
	callvec.c devopen.c getenv.c gets.c strcat.c strcmp.c strcpy.c

.PATH:	$S/lib/libsa  $S/lib/libkern

NOPROFILE=noprofile
NOPIC	=nopic
NOLINT	=nolint

# set up for standalone library compiation (turn off PIC)

.include <bsd.lib.mk>
.include "../Makefile.inc"	# XXX after to reset CFLAGS, etc
