#	$NetBSD: Makefile,v 1.2.10.1 2009/01/19 13:20:24 skrll Exp $
#

PROG=	rumptest
NOMAN=	woman

# std rump libs
LDADD=	${RUMPFSLDADD} -lrumpvfs -lrump -lrumpuser

# for NFS
LDADD+=	-lrumpnet_sockin -lrumpnet

LDADD+=	-lpthread

.include "${.CURDIR}/../Makefile.rumpfs"
.include <bsd.prog.mk>
