#	$NetBSD: Makefile,v 1.4.32.1 2020/04/08 14:03:44 martin Exp $

SUBDIR=		ramdisk

.include "../../etc/etc.evbarm/Makefile.inc"

.if !empty(KERNEL_SETS:MRPI)
SUBDIR+=	sshramdisk
.endif

SUBDIR+=	.WAIT instkernel
TARGETS+=	release

.include <bsd.subdir.mk>
