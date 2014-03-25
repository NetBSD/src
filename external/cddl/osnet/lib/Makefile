#	$NetBSD: Makefile,v 1.6 2014/03/25 09:52:55 ozaki-r Exp $
#
#

.include <bsd.own.mk>

.if (defined(MKDTRACE) && ${MKDTRACE} != "no")
SUBDIR+=	libdtrace
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
