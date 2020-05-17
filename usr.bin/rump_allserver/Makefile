#	$NetBSD: Makefile,v 1.13 2020/05/17 19:37:15 christos Exp $
#

PROG=	rump_allserver
MLINKS+=rump_allserver.1 rump_server.1

RUMPTOP=${.CURDIR}/../../sys/rump

.include "${RUMPTOP}/dev/Makefile.rumpdevcomp"
.include "${RUMPTOP}/fs/Makefile.rumpfscomp"
.include "${RUMPTOP}/kern/Makefile.rumpkerncomp"
.include "${RUMPTOP}/net/Makefile.rumpnetcomp"

LDADD+=	${RUMPDEVLDADD} ${RUMPFSLDADD} ${RUMPKERNLDADD} ${RUMPNETLDADD}
LDADD+= -lrumpdev -lrumpnet -lrumpvfs -lrump -lrumpvfs -lrumpvfs_nofifofs -lrump -lrumpuser -lpthread

.include <bsd.prog.mk>
