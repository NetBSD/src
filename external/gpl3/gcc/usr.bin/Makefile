#	$NetBSD: Makefile,v 1.3.8.1 2014/08/19 23:56:57 tls Exp $

NOOBJ=# defined

.include <bsd.own.mk>

GCC_MACHINE_ARCH=${MACHINE_ARCH:S/earmv5/earm/}

.if ${MKGCC} != "no" && exists(${.CURDIR}/gcc/arch/${GCC_MACHINE_ARCH}/defs.mk)

# We keep libcpp here since it depends upon frontend.

SUBDIR+=	host-libiberty .WAIT \
		backend .WAIT \
		frontend .WAIT \
		common common-target .WAIT \
		libcpp libdecnumber .WAIT \
		gcov cc1 cc1obj cc1plus cpp g++ gcc \
		include

.include <bsd.subdir.mk>

.else
# Do nothing. (PROG undefined)
.include <bsd.prog.mk>
.endif
