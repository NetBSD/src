#	$NetBSD: Makefile,v 1.2 2010/12/05 20:12:55 pooka Exp $
#

PROG=	rump_allserver
NOMAN=	for now

RUMPTOP=${.CURDIR}/../../sys/rump

.include "${RUMPTOP}/dev/Makefile.rumpdevcomp"
.include "${RUMPTOP}/fs/Makefile.rumpfscomp"
.include "${RUMPTOP}/kern/Makefile.rumpkerncomp"
.include "${RUMPTOP}/net/Makefile.rumpnetcomp"

LDADD+=	${RUMPDEVLDADD} ${RUMPFSLDADD} ${RUMPKERNLDADD} ${RUMPNETLDADD}
LDADD+= -lrumpdev -lrumpvfs -lrumpnet -lrump -lrumpuser -lpthread

.include <bsd.prog.mk>
