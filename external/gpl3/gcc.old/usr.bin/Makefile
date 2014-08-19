#	$NetBSD: Makefile,v 1.1.1.1.8.2 2014/08/19 23:58:23 tls Exp $

NOOBJ=# defined

.include <bsd.own.mk>

GCC_MACHINE_ARCH=${MACHINE_ARCH:S/earmv5/earm/}

.if ${MKGCC} != "no" && exists(${.CURDIR}/gcc/arch/${GCC_MACHINE_ARCH}/defs.mk)

# We keep libcpp here since it depends upon frontend.

SUBDIR+=	host-libiberty .WAIT \
		backend .WAIT \
		frontend .WAIT \
		libcpp libdecnumber gcov .WAIT \
		cc1 cc1obj cc1plus cpp g++ gcc \
		include

.include <bsd.subdir.mk>

.else
# Do nothing. (PROG undefined)
.include <bsd.prog.mk>
.endif
