#	$NetBSD: Makefile,v 1.1 2010/12/05 17:37:33 pooka Exp $
#

PROG=	rump_allserver
NOMAN=	for now

RUMPTOP=${.CURDIR}/../../sys/rump

.include "${RUMPTOP}/dev/Makefile.rumpdevcomp"
.include "${RUMPTOP}/fs/Makefile.rumpfscomp"
.include "${RUMPTOP}/kern/Makefile.rumpkerncomp"
.include "${RUMPTOP}/net/Makefile.rumpnetcomp"

LDADD+=	${RUMPDEVLDADD} ${RUMPFSLDADD} ${RUMPKERNLDADD} ${RUMPNETLDADD}
LDADD+= -lrumpcrypto -lrumpdev -lrumpvfs -lrumpnet -lrump -lrumpuser -lpthread

.include <bsd.prog.mk>
