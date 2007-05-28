# $NetBSD: Makefile,v 1.2 2007/05/28 12:06:29 tls Exp $

USE_FORT?= yes	# network client/server

PROG=		rfcomm_sppd
SRCS=		rfcomm_sppd.c rfcomm_sdp.c

DPADD+=		${LIBBLUETOOTH} ${LIBSDP}
LDADD+=		-lbluetooth -lsdp

.include <bsd.prog.mk>
