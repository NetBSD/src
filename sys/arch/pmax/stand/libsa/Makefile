#	from: @(#)Makefile	8.2 (Berkeley) 2/16/94
#       $Id: Makefile,v 1.3 1994/05/27 09:05:41 glass Exp $

LIB=	sa
SRCS=	alloc.c bcopy.c bzero.c close.c dev.c disklabel.c getfile.c \
	ioctl.c lseek.c open.c read.c ufs.c write.c \
	callvec.c devopen.c getenv.c gets.c strcat.c strcmp.c strcpy.c

.PATH:	../../../stand
NOPROFILE=noprofile

.include <bsd.lib.mk>

CFLAGS=-O2 -I/sys
