#	$NetBSD: Makefile,v 1.3 2003/09/14 09:50:15 martin Exp $

.include <bsd.own.mk>

MKPIC=no
MKPROFILE=no  
MKLINT=no	# lint misses important ifdef magic on 64 bit archs

TOP=		${NETBSDSRCDIR}/gnu
DIST=		${TOP}/dist/gdb

LIB=iberty

CPPFLAGS+=-I${.CURDIR} -I${DIST}/include -DHAVE_CONFIG_H

SRCS=argv.c concat.c cp-demangle.c cplus-dem.c dyn-string.c floatformat.c \
getopt.c getopt1.c getruntime.c hashtab.c hex.c lbasename.c objalloc.c \
obstack.c regex.c safe-ctype.c splay-tree.c xexit.c xstrdup.c xstrerror.c


libinstall::

.include "../../Makefile.inc"
.include <bsd.lib.mk>


.PATH: ${DIST}/libiberty
