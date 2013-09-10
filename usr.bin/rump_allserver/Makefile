#	$NetBSD: Makefile,v 1.6 2013/09/10 18:28:46 pooka Exp $
#

PROG=	rump_allserver
MLINKS+=rump_allserver.1 rump_server.1

RUMPTOP=${.CURDIR}/../../sys/rump

.include "${RUMPTOP}/dev/Makefile.rumpdevcomp"
.include "${RUMPTOP}/fs/Makefile.rumpfscomp"
.include "${RUMPTOP}/kern/Makefile.rumpkerncomp"
.include "${RUMPTOP}/net/Makefile.rumpnetcomp"

LDADD+=	${RUMPDEVLDADD} ${RUMPFSLDADD} ${RUMPKERNLDADD} ${RUMPNETLDADD}
LDADD+= -lrumpdev -lrumpvfs -lrumpnet -lrump -lrumpuser -lpthread

.include <bsd.own.mk>

.if ${RUMP_SERVER_LIBUTIL:Uyes} == "yes"
DPADD+=	${LIBUTIL}
LDADD+=	-lutil
.endif

.include <bsd.prog.mk>
