# $NetBSD: Makefile,v 1.4 2017/08/10 20:43:12 jmcneill Exp $

PROG=	btattach
MAN=	btattach.8
SRCS=	btattach.c firmload.c init_bcm2035.c init_bcm43xx.c init_bgb2xx.c \
	init_csr.c init_digi.c init_ericsson.c init_st.c init_stlc2500.c \
	init_swave.c init_unistone.c

DPADD+=	${LIBBLUETOOTH} ${LIBUTIL}
LDADD+=	-lbluetooth -lutil

.include <bsd.prog.mk>
