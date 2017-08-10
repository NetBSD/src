# $NetBSD: Makefile,v 1.3 2017/08/10 13:34:29 nat Exp $

PROG=	btattach
MAN=	btattach.8
SRCS=	btattach.c init_bcm2035.c init_bgb2xx.c init_csr.c init_digi.c \
	init_ericsson.c init_st.c init_stlc2500.c init_swave.c init_unistone.c \
	init_bcm43xx.c

DPADD+=	${LIBBLUETOOTH} ${LIBUTIL}
LDADD+=	-lbluetooth -lutil

.include <bsd.prog.mk>
