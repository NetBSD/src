#	$NetBSD: Makefile,v 1.3 2003/12/05 18:56:11 matt Exp $

NOOBJ=# defined

.include <bsd.own.mk>

.if ${MKGCC} != "no" && exists(${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk)

SUBDIR+=	host-libiberty libiberty .WAIT \
		backend .WAIT \
		frontend .WAIT \
		libcpp gcov .WAIT \
		cc1 cc1obj cc1plus cpp f771 g++ g77 gcc \
		include protoize unprotoize

.include <bsd.subdir.mk>

.else
# Do nothing. (PROG undefined)
.include <bsd.prog.mk>
.endif
