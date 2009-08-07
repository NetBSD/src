#	$NetBSD: Makefile,v 1.1 2009/08/07 20:57:56 haad Exp $

.include <bsd.own.mk>

.if !defined(MKZFS) || ${MKZFS} != "no"
SUBDIR+=	zpool zfs
.endif

.include <bsd.subdir.mk>
