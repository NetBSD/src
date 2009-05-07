# $NetBSD: Makefile,v 1.3 2009/05/07 18:08:55 plunky Exp $

USE_FORT?= yes	# network client/server

PROG=		rfcomm_sppd
SRCS=		rfcomm_sppd.c rfcomm_sdp.c

DPADD+=		${LIBBLUETOOTH} ${LIBSDP}
LDADD+=		-lbluetooth -lsdp

CPPFLAGS+=	-DSDP_COMPAT

.include <bsd.prog.mk>
