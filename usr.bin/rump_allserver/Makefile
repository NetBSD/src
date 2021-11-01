#	$NetBSD: Makefile,v 1.15 2021/11/01 21:37:33 nia Exp $
#

PROG=	rump_allserver
MLINKS+=rump_allserver.1 rump_server.1

NOFULLRELRO=	yes

RUMPTOP=${.CURDIR}/../../sys/rump

.include "${RUMPTOP}/dev/Makefile.rumpdevcomp"
.include "${RUMPTOP}/fs/Makefile.rumpfscomp"
.include "${RUMPTOP}/kern/Makefile.rumpkerncomp"
.include "${RUMPTOP}/net/Makefile.rumpnetcomp"

LDADD+=	${RUMPDEVLDADD} ${RUMPFSLDADD} ${RUMPKERNLDADD} ${RUMPNETLDADD}
LDADD+= -lrumpdev -lrumpnet -lrumpvfs -lrump -lrumpvfs -lrumpvfs_nofifofs -lrump -lrumpuser -lpthread

.if ${RUMP_SANITIZE:Uno} != "no"
LDADD+=	-fsanitize=${RUMP_SANITIZE}
.endif

.include <bsd.prog.mk>
