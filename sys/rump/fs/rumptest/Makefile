#	$NetBSD: Makefile,v 1.2.8.3 2008/12/13 01:15:34 haad Exp $
#

PROG=	rumptest
NOMAN=	woman

# std rump libs
LDADD=	${RUMPFSLDADD} -lrumpvfs -lrump -lrumpuser

# for NFS
LDADD+=	-lrumpnet_sockin -lrumpnet

LDADD+=	-lpthread

LDFLAGS+=-Wl,--wrap=malloc

.include "${.CURDIR}/../Makefile.rumpfs"
.include <bsd.prog.mk>
