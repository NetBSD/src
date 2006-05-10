#	$NetBSD: Makefile,v 1.1 2006/05/10 22:48:11 mrg Exp $

NOOBJ=# defined

.include <bsd.own.mk>

.if ${MKGCC} != "no" && exists(${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk)

SUBDIR+=	host-libiberty libiberty .WAIT \
		backend .WAIT \
		frontend .WAIT \
		libcpp gcov .WAIT \
		cc1 cc1obj cc1plus cpp g++ gcc \
		include protoize unprotoize

.include <bsd.subdir.mk>

.else
# Do nothing. (PROG undefined)
.include <bsd.prog.mk>
.endif
