#	$NetBSD: Makefile,v 1.14 2001/12/12 01:49:06 tv Exp $

.include "../Makefile.comp"

DIST=		${IDIST}/src/libs/libdriver
.PATH:		${DIST}

LIB=		driver
SRCS=		printer.cc input.cc
# lint step doesn't work, because there are no .ln files here, because
# we're only compiling c++ files in this dir
# lint command line is: lint -Cdriver -lc
NOLINT=		# defined
NOMAN=		# defined
NOPROFILE=	# defined
NOPIC=		# defined

# only needed during build
libinstall::

.include <bsd.lib.mk>
