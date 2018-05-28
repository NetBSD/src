#	$NetBSD: Makefile,v 1.9 2018/05/28 21:05:08 chs Exp $
#
#

.include <bsd.own.mk>

.if (defined(MKDTRACE) && ${MKDTRACE} != "no")
SUBDIR+=	libdtrace
SUBDIR+=	drti
.endif
.if ${MKCTF:Uno} != "no"
SUBDIR+=	libctf
.endif

.if !defined(MKZFS) || ${MKZFS} != "no"
SUBDIR+=	libavl
SUBDIR+=	libnvpair
SUBDIR+=	libumem
SUBDIR+=	libuutil
SUBDIR+=	.WAIT
SUBDIR+=	libzfs_core
SUBDIR+=	.WAIT
SUBDIR+=	libzfs
SUBDIR+=	libzpool
.endif

.include <bsd.subdir.mk>
