#	$NetBSD: Makefile,v 1.5 2009/01/14 19:29:32 pooka Exp $
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
