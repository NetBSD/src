#	$NetBSD: Makefile.inc,v 1.28 2001/01/08 08:29:59 itojun Exp $

.if !defined(GCC_MAKEFILE_INC)
GCC_MAKEFILE_INC=1

.include <bsd.own.mk>

# These architectures are Cygnus's default for enabling Haifa
.if (${MACHINE_ARCH} == "alpha") || (${MACHINE_ARCH} == "powerpc") || \
    (${MACHINE_ARCH} == "sparc") || (${MACHINE_ARCH} == "sparc64")
USE_EGCS_HAIFA=1
.endif
#MKMAN=no

# gcc has files with a "rs6000" prefix for powerpc
GNU_ARCH.powerpc=rs6000

DIST=		${.CURDIR}/../../../dist/gcc
COMMON=		${.CURDIR}/../common
.ifnmake(print-objdir)
COMMONOBJ!=	cd ${.CURDIR}/../common; ${MAKE} print-objdir
.endif

.if exists(${.CURDIR}/../arch/${MACHINE_ARCH})
ARCHSUBDIR= ${MACHINE_ARCH}
.elif exists(${.CURDIR}/../arch/${MACHINE_CPU})
ARCHSUBDIR= ${MACHINE_CPU}
.else
.BEGIN:
	@echo no ../arch/${MACHINE_ARCH} nor ../arch/${MACHINE_CPU} exists
	@false
.endif

CPPFLAGS+=	-I${.CURDIR}/../arch \
		-I${.CURDIR}/../arch/${ARCHSUBDIR} \
		-I${COMMONOBJ} \
		-I${DIST} \
		-I${DIST}/config \
		-DIN_GCC -DNETBSD_NATIVE
HOST_CPPFLAGS+= -I${.CURDIR}/../arch \
		-I${.CURDIR}/../arch/${ARCHSUBDIR} \
		-I${COMMONOBJ} \
		-I${DIST} \
		-I${DIST}/config \
		-DIN_GCC -DNETBSD_NATIVE

.if (${OBJECT_FMT} == "ELF" || defined(BOOTSTRAP_ELF))
CPPFLAGS+=-DDEFAULT_ELF
.endif

.if (${MACHINE_ARCH} == "mipseb") || (${MACHINE_ARCH} == "sh3eb")
CPPFLAGS+=-DTARGET_BIG_ENDIAN_DEFAULT
.endif

.if (${MACHINE_ARCH} == "mipsel") || (${MACHINE_ARCH} == "sh3el")
CPPFLAGS+=-DTARGET_LITTLE_ENDIAN_DEFAULT
.endif

.PATH:		${.CURDIR}/../arch \
		${DIST} \
		${DIST}/config

EGCS_VERSION!=	sed -e 's/.*\(egcs-[0-9\.]*\).*/\1/' ${DIST}/version.c

.if !defined(COMPILER_EXEC_PREFIX)
COMPILER_EXEC_PREFIX=
.endif

CPPFLAGS+=	-DHAVE_CONFIG_H -DPREFIX=\"/usr\" \
		-DDEFAULT_TARGET_VERSION=\"${EGCS_VERSION}\" \
		-DDEFAULT_TARGET_MACHINE=\"${MACHINE_GNU_ARCH}-netbsd\" \
		-DSTANDARD_STARTFILE_PREFIX=\"/usr/lib/\" \
		-DSTANDARD_EXEC_PREFIX=\"/usr/libexec/\" \
		-DTOOLDIR_BASE_PREFIX=\"/usr/libexec/\" \
		-DNBSD_EXEC_PREFIX=\"${COMPILER_EXEC_PREFIX}/usr/libexec/\"

.if defined(USE_EGCS_HAIFA)
CPPFLAGS+=	-DHAIFA
.endif
.endif
