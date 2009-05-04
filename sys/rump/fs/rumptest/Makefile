#	$NetBSD: Makefile,v 1.5.4.2 2009/05/04 08:14:28 yamt Exp $
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
