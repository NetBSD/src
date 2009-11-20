#	$NetBSD: Makefile,v 1.6 2009/11/20 15:37:44 pooka Exp $
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
