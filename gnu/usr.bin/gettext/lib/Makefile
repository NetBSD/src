#	$NetBSD: Makefile,v 1.2 2001/12/12 01:49:04 tv Exp $

.include <bsd.own.mk>

LIB=	nlsut
SRCS=	basename.c error.c fstrcmp.c getline.c getopt.c getopt1.c hash.c \
	obstack.c printf-prs.c stpcpy.c stpncpy.c xgetcwd.c xmalloc.c \
	xstrdup.c

NOPIC=# defined
NOPROFILE=# defined
NOLINT=# defined

.include "${.CURDIR}/../Makefile.inc"

.PATH:	${DIST}/gettext/lib

# only needed during build
libinstall::

.include <bsd.lib.mk>
