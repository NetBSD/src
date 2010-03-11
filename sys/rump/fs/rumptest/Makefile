#	$NetBSD: Makefile,v 1.5.4.3 2010/03/11 15:04:37 yamt Exp $
#

PROG=	rumptest
NOMAN=	woman

# std rump libs
LDADD=	${RUMPFSLDADD} -lrumpvfs -lrump -lrumpuser

# for NFS
LDADD+=	-lrumpnet_sockin -lrumpnet

# for SMBFS
LDADD+=	-lrumpcrypto -lrumpdev_netsmb -lrumpdev

LDADD+=	-lpthread

.include "${.CURDIR}/../Makefile.rumpfs"
.include <bsd.prog.mk>
