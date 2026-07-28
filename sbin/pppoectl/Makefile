#	$NetBSD: Makefile,v 1.7 2026/07/28 07:28:02 yamaguchi Exp $

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
