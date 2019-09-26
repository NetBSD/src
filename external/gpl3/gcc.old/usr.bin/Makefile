#	$NetBSD: Makefile,v 1.10 2019/09/26 08:03:59 mrg Exp $

NOOBJ=# defined

.include <bsd.init.mk>

GCC_MACHINE_ARCH=${MACHINE_ARCH:S/earmv5/earm/}

.if ${MKGCC} != "no" && exists(${.CURDIR}/gcc/arch/${GCC_MACHINE_ARCH}/defs.mk)

.if ${MKGCCCMDS} != "no" || make(includes)

# We keep libcpp here since it depends upon frontend.
SUBDIR+=	host-libiberty .WAIT \
		host-libcpp .WAIT \
		backend .WAIT \
		frontend .WAIT \
		common common-target .WAIT \
		libcpp libdecnumber .WAIT \
		gcov gcov-dump cc1 cc1obj cc1plus cpp g++ gcc \
		lto1 lto-wrapper \
		include

.endif

.include <bsd.subdir.mk>

.else
# Do nothing. (PROG undefined)
.include <bsd.prog.mk>
.endif
