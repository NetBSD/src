#	$NetBSD: Makefile,v 1.4 2004/05/23 02:24:51 lukem Exp $

.include <bsd.own.mk>

MKPRIVATELIB=	yes

TOP=		${NETBSDSRCDIR}/gnu
DIST=		${TOP}/dist/gdb

LIB=iberty

CPPFLAGS+=-I${.CURDIR} -I${DIST}/include -DHAVE_CONFIG_H

SRCS=argv.c concat.c cp-demangle.c cplus-dem.c dyn-string.c floatformat.c \
getopt.c getopt1.c getruntime.c hashtab.c hex.c lbasename.c objalloc.c \
obstack.c regex.c safe-ctype.c splay-tree.c xexit.c xstrdup.c xstrerror.c

.include "../../Makefile.inc"
.include <bsd.lib.mk>


.PATH: ${DIST}/libiberty
