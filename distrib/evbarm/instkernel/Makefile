#	$NetBSD: Makefile,v 1.5 2020/03/26 08:02:55 skrll Exp $

SUBDIR=		ramdisk

.if !empty(ALL_KERNELS:MRPI_INSTALL)
SUBDIR+=	sshramdisk
.endif

SUBDIR+=	.WAIT instkernel
TARGETS+=	release

.include <bsd.subdir.mk>
