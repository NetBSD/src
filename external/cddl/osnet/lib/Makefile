#	$NetBSD: Makefile,v 1.4 2010/02/24 12:18:37 tron Exp $
#
#

.include <bsd.own.mk>

.if (defined(MKDTRACE) && ${MKDTRACE} != "no")
SUBDIR+=	libdtrace
SUBDIR+=	libctf
.endif

.if !defined(MKZFS) || ${MKZFS} != "no"
SUBDIR+=	libavl
SUBDIR+=	libnvpair
SUBDIR+=	libumem
SUBDIR+=	libuutil
SUBDIR+=	libzfs
SUBDIR+=	libzpool
.endif

.include <bsd.subdir.mk>
