#	$NetBSD: Makefile,v 1.15 2002/09/19 03:09:41 lukem Exp $

# lint step doesn't work, because there are no .ln files here, because
# we're only compiling c++ files in this dir
# lint command line is: lint -Cdriver -lc
NOLINT=		# defined
NOMAN=		# defined
NOPROFILE=	# defined
NOPIC=		# defined

.include "../Makefile.comp"

DIST=		${IDIST}/src/libs/libdriver
.PATH:		${DIST}

LIB=		driver
SRCS=		printer.cc input.cc

# only needed during build
libinstall::

.include <bsd.lib.mk>
