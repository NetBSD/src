#	$NetBSD: Makefile,v 1.6 2024/06/05 16:37:44 riastradh Exp $

SUBDIR=
SUBDIR+=	ramdisk
SUBDIR+=	ramdisk-cgdroot
SUBDIR+=	${${MKZFS} != "no":?ramdisk-zfsroot:}

TARGETS+=	release

.include <bsd.subdir.mk>
