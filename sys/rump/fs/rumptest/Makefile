#	$NetBSD: Makefile,v 1.4 2008/11/27 11:31:59 pooka Exp $
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
