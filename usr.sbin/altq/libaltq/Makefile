#	$NetBSD: Makefile,v 1.7 2005/01/10 02:58:59 lukem Exp $

MKPRIVATELIB=	yes

.include <bsd.own.mk>

#WARNS=	2

LIB=	altq
SRCS=	parser.c qop.c qop_blue.c qop_cbq.c qop_cdnr.c qop_conf.c \
	qop_dummy.c qop_errlist.c qop_fifoq.c qop_hfsc.c qop_priq.c \
	qop_red.c qop_rio.c qop_wfq.c quip_server.c

CPPFLAGS+= -DALTQ

.if (${USE_INET6} != "no")
CPPFLAGS+= -DINET6
.endif

.include <bsd.lib.mk>
