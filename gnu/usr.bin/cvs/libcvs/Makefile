#	$NetBSD: Makefile,v 1.2 2000/09/04 22:14:31 christos Exp $

.include "${.CURDIR}/../Makefile.inc"
LIB=	cvs

DIST=	${IDIST}/lib
.PATH:	${DIST}

SRCS=	argmatch.c dup2.c getdate.y getline.c getopt.c getopt1.c \
	md5.c regex.c savecwd.c sighandle.c stripslash.c xgetwd.c \
	yesno.c

MKLINT=		no
MKPIC=		no
MKPROFILE=	no

# only needed during build
libinstall::

.include <bsd.lib.mk>
