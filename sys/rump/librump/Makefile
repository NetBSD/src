#	$NetBSD: Makefile,v 1.7 2009/11/17 15:36:28 pooka Exp $
#

SUBDIR= 	rumpkern
.if !defined(NORUMPUSER)
SUBDIR+=	rumpuser
.endif

SUBDIR+=	rumpcrypto rumpdev rumpnet rumpvfs

.include <bsd.subdir.mk>
