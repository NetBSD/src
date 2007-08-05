#	$NetBSD: Makefile,v 1.1 2007/08/05 22:28:02 pooka Exp $
#

PROG=		ffs

LDADD+=		${RUMPFSLD_FFS} ${RUMPFSLD_UFS}
DPADD+=		${RUMPFSDP_FFS} ${RUMPFSDP_UFS}

RUMPFS_WANT+=	ufs

.include <bsd.prog.mk>
