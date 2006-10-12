#	$NetBSD: Makefile,v 1.3 2006/10/12 19:59:13 peter Exp $

PROG=	altqstat
SRCS=	altqstat.c quip_client.c qdisc_conf.c qdisc_cbq.c qdisc_hfsc.c \
	qdisc_cdnr.c qdisc_wfq.c qdisc_fifoq.c qdisc_red.c qdisc_rio.c \
	qdisc_blue.c qdisc_priq.c qdisc_jobs.c

CPPFLAGS+= -DALTQ -I${.CURDIR}/../libaltq

DPADD+=	${LIBCURSES} ${LIBMATH}
LDADD+=	-lcurses -lm

.include <bsd.prog.mk>
