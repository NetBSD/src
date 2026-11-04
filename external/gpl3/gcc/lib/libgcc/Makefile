#	$NetBSD: Makefile,v 1.5.28.1 2026/11/04 22:52:30 perseant Exp $

.include <bsd.own.mk>

SUBDIR+=	libgcc

.if ${MKPIC} != "no"
# libgcc_s depends upon libgcc.a
SUBDIR+=	libgcc_eh .WAIT libgcc_s
.endif

SUBDIR+=	libgcov

.include <bsd.subdir.mk>
