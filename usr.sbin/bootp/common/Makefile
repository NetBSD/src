#	$NetBSD: Makefile,v 1.5 2002/09/18 13:31:52 lukem Exp $

NOLINT=		# defined
NOPIC=		# defined
NOPROFILE=	# defined

LIB=	common
SRCS=	dovend.c dumptab.c getif.c hash.c hwaddr.c lookup.c readfile.c \
	report.c tzone.c
CPPFLAGS+= -DETC_ETHERS -DSYSLOG -DDEBUG

# only needed during build
libinstall::

.include <bsd.lib.mk>
