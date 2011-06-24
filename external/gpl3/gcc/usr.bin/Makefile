#	$NetBSD: Makefile,v 1.2 2011/06/24 05:25:24 mrg Exp $

NOOBJ=# defined

.include <bsd.own.mk>

.if ${MKGCC} != "no" && exists(${.CURDIR}/gcc/arch/${MACHINE_ARCH}/defs.mk)

# We keep libcpp here since it depends upon frontend.

SUBDIR+=	host-libiberty .WAIT \
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
