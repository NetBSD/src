#	$NetBSD: Makefile,v 1.4 2004/05/23 02:24:51 lukem Exp $

MKPRIVATELIB=	yes

.include <bsd.own.mk>

LIB=	nlsut
SRCS=	basename.c error.c fstrcmp.c getline.c getopt.c getopt1.c hash.c \
	obstack.c printf-prs.c stpcpy.c stpncpy.c xgetcwd.c xmalloc.c \
	xstrdup.c

.include "${.CURDIR}/../Makefile.inc"

.PATH:	${DIST}/gettext/lib

.include <bsd.lib.mk>
