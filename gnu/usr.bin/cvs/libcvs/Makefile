#	$NetBSD: Makefile,v 1.4 2001/12/12 12:24:19 lukem Exp $

NOLINT=		# defined
NOPIC=		# defined
NOPROFILE=	# defined

.include "${.CURDIR}/../Makefile.inc"
LIB=	cvs

DIST=	${IDIST}/lib
.PATH:	${DIST}

SRCS=	argmatch.c dup2.c getdate.y getline.c getopt.c getopt1.c \
	md5.c regex.c savecwd.c sighandle.c stripslash.c xgetwd.c \
	yesno.c

# only needed during build
libinstall::

.include <bsd.lib.mk>
