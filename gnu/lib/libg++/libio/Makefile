#	$NetBSD: Makefile,v 1.1.1.1 1996/03/09 00:01:31 phil Exp $

LIB=	io

SRCS=	cleanup.c filedoalloc.c fileops.c floatconv.c genops.c iofclose.c \
iofdopen.c iofflush.c iofgetpos.c iofgets.c iofopen.c iofprintf.c \
iofputs.c iofread.c iofscanf.c iofsetpos.c ioftell.c iofwrite.c \
iogetdelim.c iogetline.c iogets.c ioignore.c iopadn.c ioperror.c \
iopopen.c ioprims.c ioprintf.c ioputs.c ioscanf.c ioseekoff.c \
ioseekpos.c iosetbuffer.c iosetvbuf.c iosprintf.c iosscanf.c iostrerror.c \
ioungetc.c iovfprintf.c iovfscanf.c iovsprintf.c iovsscanf.c outfloat.c \
stdfiles.c strops.c 

CXXFLAGS+=	-nostdinc++ -I$(.CURDIR)/../include 
CFLAGS+=	-I$(.CURDIR)
NOMAN=		noman

.include <bsd.lib.mk>
