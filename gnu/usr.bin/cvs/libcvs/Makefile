#	$NetBSD: Makefile,v 1.5.2.1 2005/07/06 20:12:40 snj Exp $

MKPRIVATELIB=	yes

.include "${.CURDIR}/../Makefile.inc"

LIB=	cvs

DIST=	${IDIST}/lib
.PATH:	${DIST}

SRCS=	argmatch.c getdate.y getline.c getopt.c getopt1.c \
	md5.c regex.c savecwd.c sighandle.c stripslash.c xgetwd.c \
	yesno.c

.include <bsd.lib.mk>
