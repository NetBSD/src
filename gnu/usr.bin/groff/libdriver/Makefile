#	$NetBSD: Makefile,v 1.13 2001/04/19 14:41:25 wiz Exp $

.include "../Makefile.comp"

DIST=		${IDIST}/src/libs/libdriver
.PATH:		${DIST}

LIB=		driver
SRCS=		printer.cc input.cc
# lint step doesn't work, because there are no .ln files here, because
# we're only compiling c++ files in this dir
# lint command line is: lint -Cdriver -lc
MKLINT=		no
MKMAN=		no
MKPROFILE=	no
MKPIC=		no

# only needed during build
libinstall::

.include <bsd.lib.mk>
