#	$NetBSD: Makefile,v 1.3 2001/12/12 01:48:48 tv Exp $

#WARNS=	2

LIB=	altq
SRCS=	parser.c qop.c qop_blue.c qop_cbq.c qop_cdnr.c qop_conf.c \
	qop_dummy.c qop_errlist.c qop_fifoq.c qop_hfsc.c qop_priq.c \
	qop_red.c qop_rio.c qop_wfq.c quip_server.c

CPPFLAGS+= -DALTQ

NOLINT=		# defined
NOPIC=		# defined
NOPROFILE=	# defined

# only needed during build
libinstall::

.include <bsd.lib.mk>
