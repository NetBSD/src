#	$NetBSD: Makefile,v 1.20 2026/01/19 02:45:14 mrg Exp $

NOOBJ=# defined

.include <bsd.init.mk>

GCC_MACHINE_ARCH?=${MACHINE_ARCH:S/earmv5/earm/}

.if ${MKGCC} != "no" && exists(${.CURDIR}/gcc/arch/${GCC_MACHINE_ARCH}/defs.mk)

.if !defined(NOGCCISL)
ISL_SUBDIR=	../../../mit/isl/lib/libisl
.else
ISL_SUBDIR=
.endif

# host-libiberty is used by include on rs6000.
SUBDIR+=	host-libiberty .WAIT

# We keep libcpp here since it depends upon frontend.
.if ${MKGCCCMDS} != "no" || make(includes)
SUBDIR+=	host-libcpp .WAIT \
		${ISL_SUBDIR} \
		backend .WAIT \
		frontend .WAIT \
		common common-target .WAIT \
		libcody libcpp libdecnumber .WAIT \
		gcov gcov-dump cc1 cc1obj cc1objplus cc1plus f951 lto1 \
		cpp g++ gcc gfortran lto-dump lto-wrapper
.endif

SUBDIR+=	include

# Make sure that we do host-liberty before include, rs6000 has a generator
# for an installed a header and the generator needs host-libiberty.
include-include: include-host-libiberty

.include <bsd.subdir.mk>

.else
# Do nothing. (PROG undefined)
.include <bsd.prog.mk>
.endif
