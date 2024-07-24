#	$NetBSD: Makefile,v 1.4.20.2 2024/07/24 09:52:52 martin Exp $

SUBDIR=
SUBDIR+=	ramdisk
SUBDIR+=	ramdisk-cgdroot
SUBDIR+=	${${MKZFS} != "no":?ramdisk-zfsroot:}

TARGETS+=	release

.include <bsd.subdir.mk>
