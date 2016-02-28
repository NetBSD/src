#	$NetBSD: Makefile,v 1.7 2016/02/28 19:10:10 christos Exp $
#
#

.include <bsd.own.mk>

.if (defined(MKDTRACE) && ${MKDTRACE} != "no")
SUBDIR+=	libdtrace
SUBDIR+=	drti
.endif
.if ${MKDTRACE:Uno} != "no" || ${MKCTF:Uno} != "no"
SUBDIR+=	libctf
.endif

.if !defined(MKZFS) || ${MKZFS} != "no"
SUBDIR+=	libavl
SUBDIR+=	libnvpair
SUBDIR+=	libumem
SUBDIR+=	libuutil
SUBDIR+=	.WAIT
SUBDIR+=	libzfs
SUBDIR+=	libzpool
.endif

.include <bsd.subdir.mk>
