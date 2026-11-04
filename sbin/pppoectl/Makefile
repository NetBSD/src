#	$NetBSD: Makefile,v 1.6.2.1 2026/11/04 23:19:42 perseant Exp $

PROG=	pppoectl
MAN=	pppoectl.8

USE_FORT?= yes	# network client

COPTS.pppoectl.c+=	${CC_WNO_STRINGOP_TRUNCATION}

.if !defined(RESCUEDIR)
# SPPP_FILTER support
LDADD=		-lpcap
CPPFLAGS+=	-DSPPP_FILTER
.endif

.include <bsd.prog.mk>
