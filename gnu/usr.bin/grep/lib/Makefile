#	$NetBSD: Makefile,v 1.1 2003/01/26 23:53:47 wiz Exp $

NOLINT=		# defined
NOPIC=		# defined
NOPROFILE=	# defined

.include "${.CURDIR}/../Makefile.inc"

LIB=	greputils

DIST=	${IDIST}/lib
.PATH:	${DIST}

SRCS=	closeout.c error.c exclude.c hard-locale.c isdir.c \
	obstack.c quotearg.c regex.c savedir.c stpcpy.c \
	xmalloc.c xstrtol.c xstrtoumax.c

# only needed during build
libinstall::

.include <bsd.lib.mk>
