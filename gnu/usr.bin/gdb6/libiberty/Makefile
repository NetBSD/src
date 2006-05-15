#	$NetBSD: Makefile,v 1.1 2006/05/15 16:31:37 nathanw Exp $

.include <bsd.own.mk>

MKPRIVATELIB=	yes

TOP=		${NETBSDSRCDIR}/gnu
DIST=		${TOP}/dist/gdb6

LIB=iberty

CPPFLAGS+=-I${.CURDIR} -I${DIST}/include -DHAVE_CONFIG_H

SRCS=argv.c concat.c cp-demangle.c cp-demint.c cplus-dem.c		\
dyn-string.c floatformat.c getopt.c getopt1.c getruntime.c hashtab.c	\
hex.c lbasename.c objalloc.c obstack.c regex.c safe-ctype.c		\
splay-tree.c unlink-if-ordinary.c xexit.c xstrdup.c xstrerror.c

.include "../../Makefile.inc"
.include <bsd.lib.mk>


.PATH: ${DIST}/libiberty
