#	$NetBSD: Makefile,v 1.1.1.1 2003/01/26 00:40:40 wiz Exp $

NOLINT=		# defined
NOPIC=		# defined
NOPROFILE=	# defined

.include "${.CURDIR}/../Makefile.inc"

LIB=	diffutils

DIST=	${IDIST}/lib
.PATH:	${DIST}

SRCS=	basename.c c-stack.c cmpbuf.c error.c exclude.c exitfail.c \
	freesoft.c \
	hard-locale.c imaxtostr.c offtostr.c prepargs.c posixver.c quotesys.c \
	regex.c setmode.c strftime.c umaxtostr.c xmalloc.c xstrtoumax.c

# only needed during build
libinstall::

.include <bsd.lib.mk>
