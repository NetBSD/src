#	$NetBSD: Makefile,v 1.1.2.2 2007/08/15 13:50:12 skrll Exp $
#

PROG=		ffs

LDADD+=		${RUMPFSLD_FFS} ${RUMPFSLD_UFS}
DPADD+=		${RUMPFSDP_FFS} ${RUMPFSDP_UFS}

RUMPFS_WANT+=	ufs

.include <bsd.prog.mk>
