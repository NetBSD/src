#	$NetBSD: Makefile,v 1.3 2001/12/12 12:24:20 lukem Exp $

NOLINT=		# defined
NOPIC=		# defined
NOPROFILE=	# defined

.include <bsd.own.mk>

LIB=	nlsut
SRCS=	basename.c error.c fstrcmp.c getline.c getopt.c getopt1.c hash.c \
	obstack.c printf-prs.c stpcpy.c stpncpy.c xgetcwd.c xmalloc.c \
	xstrdup.c

.include "${.CURDIR}/../Makefile.inc"

.PATH:	${DIST}/gettext/lib

# only needed during build
libinstall::

.include <bsd.lib.mk>
