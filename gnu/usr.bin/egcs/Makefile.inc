#	$NetBSD: Makefile.inc,v 1.25 2000/03/05 05:41:05 shin Exp $

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

.if (${MACHINE_ARCH} == "mipsel") || (${MACHINE_ARCH} == "mipseb")
ARCHSUBDIR= mips
.else
ARCHSUBDIR= ${MACHINE_ARCH}
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

.if (${MACHINE_ARCH} == "mipseb")
CPPFLAGS+=-DTARGET_BIG_ENDIAN_DEFAULT
.endif

.if (${MACHINE_ARCH} == "mipsel")
CPPFLAGS+=-DTARGET_LITTLE_ENDIAN_DEFAULT
.endif

.PATH:		${.CURDIR}/../arch \
		${DIST} \
		${DIST}/config

EGCS_VERSION!=	sed -e 's/.*\(egcs-[0-9\.]*\).*/\1/' ${DIST}/version.c

CPPFLAGS+=	-DHAVE_CONFIG_H -DPREFIX=\"/usr\" \
		-DDEFAULT_TARGET_VERSION=\"${EGCS_VERSION}\" \
		-DDEFAULT_TARGET_MACHINE=\"${MACHINE_GNU_ARCH}-netbsd\" \
		-DSTANDARD_STARTFILE_PREFIX=\"/usr/lib/\" \
		-DSTANDARD_EXEC_PREFIX=\"/usr/libexec/\" \
		-DTOOLDIR_BASE_PREFIX=\"/usr/libexec/\"

.if defined(USE_EGCS_HAIFA)
CPPFLAGS+=	-DHAIFA
.endif
.endif
