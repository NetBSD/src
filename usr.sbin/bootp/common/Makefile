#	$NetBSD: Makefile,v 1.4 2001/12/12 01:48:49 tv Exp $

LIB=	common
SRCS=	dovend.c dumptab.c getif.c hash.c hwaddr.c lookup.c readfile.c \
	report.c tzone.c
CPPFLAGS+= -DETC_ETHERS -DSYSLOG -DDEBUG

NOLINT=		# defined
NOPIC=		# defined
NOPROFILE=	# defined

# only needed during build
libinstall::

.include <bsd.lib.mk>
