#	$NetBSD: Makefile,v 1.1 2000/11/02 01:09:03 itojun Exp $

.include <bsd.own.mk>

LIB=	nlsut
SRCS=	basename.c error.c fstrcmp.c getline.c getopt.c getopt1.c hash.c \
	obstack.c printf-prs.c stpcpy.c stpncpy.c xgetcwd.c xmalloc.c \
	xstrdup.c

MKPIC=no
MKPROFILE=no
MKLINT=no

.include "${.CURDIR}/../Makefile.inc"

.PATH:	${DIST}/gettext/lib

# only needed during build
libinstall::

.include <bsd.lib.mk>
