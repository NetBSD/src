#	$NetBSD: Makefile,v 1.1 2003/07/25 16:32:23 mrg Exp $

NOOBJ=# defined

.include <bsd.own.mk>

.if ${MKGCC} != "no" && exists(${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk)

SUBDIR+=	host-libiberty libiberty .WAIT backend frontend .WAIT libcpp gcov .WAIT \
		cc1 cc1obj cc1plus cpp f771 g++ g77 gcc protoize unprotoize

.include <bsd.subdir.mk>

.else
# Do nothing. (PROG undefined)
.include <bsd.prog.mk>
.endif
