#	$NetBSD: Makefile,v 1.2 2003/10/11 05:16:18 enami Exp $

NOOBJ=# defined

.include <bsd.own.mk>

.if ${MKGCC} != "no" && exists(${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk)

SUBDIR+=	host-libiberty libiberty .WAIT backend .WAIT \
		frontend .WAIT libcpp gcov .WAIT \
		cc1 cc1obj cc1plus cpp f771 g++ g77 gcc protoize unprotoize

.include <bsd.subdir.mk>

.else
# Do nothing. (PROG undefined)
.include <bsd.prog.mk>
.endif
