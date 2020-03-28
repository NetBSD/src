#	$NetBSD: Makefile,v 1.6 2020/03/28 15:41:23 martin Exp $

SUBDIR=		ramdisk

.include "../../etc/etc.evbarm/Makefile.inc"

.if !empty(KERNEL_SETS:MRPI)
SUBDIR+=	sshramdisk
.endif

SUBDIR+=	.WAIT instkernel
TARGETS+=	release

.include <bsd.subdir.mk>
