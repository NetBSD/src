#	$NetBSD: Makefile,v 1.3 2001/12/12 01:49:03 tv Exp $

.include "${.CURDIR}/../Makefile.inc"
LIB=	cvs

DIST=	${IDIST}/lib
.PATH:	${DIST}

SRCS=	argmatch.c dup2.c getdate.y getline.c getopt.c getopt1.c \
	md5.c regex.c savecwd.c sighandle.c stripslash.c xgetwd.c \
	yesno.c

NOLINT=		# defined
NOPIC=		# defined
NOPROFILE=	# defined

# only needed during build
libinstall::

.include <bsd.lib.mk>
