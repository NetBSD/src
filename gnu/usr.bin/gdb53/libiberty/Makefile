#	$NetBSD: Makefile,v 1.4.10.1 2006/12/06 13:37:42 tron Exp $

.include <bsd.own.mk>

MKPRIVATELIB=	yes

TOP=		${NETBSDSRCDIR}/gnu
DIST=		${TOP}/dist/gdb

LIB=iberty

CPPFLAGS+=-I${.CURDIR} -I${DIST}/include -DHAVE_CONFIG_H

SRCS=argv.c concat.c cp-demangle.c cplus-dem.c dyn-string.c floatformat.c \
getopt.c getopt1.c getruntime.c hashtab.c hex.c lbasename.c objalloc.c \
obstack.c regex.c safe-ctype.c splay-tree.c xexit.c xstrdup.c xstrerror.c

COPTS.argv.c = -Wno-stack-protector
COPTS.cp-demangle.c = -Wno-stack-protector
COPTS.make-relative-prefix.c = -Wno-stack-protector
COPTS.regex.c = -Wno-stack-protector

.include "../../Makefile.inc"
.include <bsd.lib.mk>


.PATH: ${DIST}/libiberty
