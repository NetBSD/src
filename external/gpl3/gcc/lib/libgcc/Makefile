#	$NetBSD: Makefile,v 1.5.30.1 2025/11/15 10:11:09 martin Exp $

.include <bsd.own.mk>

SUBDIR+=	libgcc

.if ${MKPIC} != "no"
# libgcc_s depends upon libgcc.a
SUBDIR+=	libgcc_eh .WAIT libgcc_s
.endif

SUBDIR+=	libgcov

.include <bsd.subdir.mk>
