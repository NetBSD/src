# $NetBSD: Makefile,v 1.1 2008/04/15 11:17:48 plunky Exp $

PROG=	btattach
MAN=	btattach.8
SRCS=	btattach.c init_bcm2035.c init_bgb2xx.c init_csr.c init_digi.c \
	init_ericsson.c init_st.c init_stlc2500.c init_swave.c

DPADD+=	${LIBBLUETOOTH} ${LIBUTIL}
LDADD+=	-lbluetooth -lutil

.include <bsd.prog.mk>
