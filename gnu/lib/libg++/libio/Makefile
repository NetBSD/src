#	$NetBSD: Makefile,v 1.3 1997/10/25 09:27:22 lukem Exp $

LIB=	io

SRCS=	chr.cc cleanup.c filedoalloc.c fileops.c floatconv.c genops.c \
iofclose.c iofdopen.c iofflush.c iofgetpos.c iofgets.c iofopen.c \
iofprintf.c iofputs.c iofread.c iofscanf.c iofsetpos.c ioftell.c \
iofwrite.c iogetdelim.c iogetline.c iogets.c ioignore.c iopadn.c \
ioperror.c iopopen.c ioprims.c ioprintf.c ioputs.c ioscanf.c \
ioseekoff.c ioseekpos.c iosetbuffer.c iosetvbuf.c iosprintf.c iosscanf.c \
iostrerror.c ioungetc.c iovfprintf.c iovfscanf.c iovsprintf.c iovsscanf.c \
outfloat.c str.cc stdfiles.c strops.c 

CXXFLAGS+=	-nostdinc++ -I$(.CURDIR)/../include 
CPPFLAGS+=	-I$(.CURDIR)
NOMAN=		noman
NOLINT=		nolint

.include <bsd.lib.mk>
