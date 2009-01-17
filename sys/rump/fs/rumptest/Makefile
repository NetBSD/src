#	$NetBSD: Makefile,v 1.2.6.3 2009/01/17 13:29:35 mjf Exp $
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
