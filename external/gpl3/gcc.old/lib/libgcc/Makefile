#	$NetBSD: Makefile,v 1.10.2.1 2026/11/04 23:00:10 perseant Exp $

.include <bsd.own.mk>

SUBDIR+=	libgcc

.if ${MKPIC} != "no"
# libgcc_s depends upon libgcc.a
SUBDIR+=	libgcc_eh .WAIT libgcc_s
.endif

SUBDIR+=	libgcov

.include <bsd.subdir.mk>
