#	$NetBSD: Makefile.inc,v 1.7 1998/05/07 04:17:59 mycroft Exp $

.if !defined(GCC_MAKEFILE_INC)
GCC_MAKEFILE_INC=1

DIST=	${.CURDIR}/../../../dist/gcc
COMMON=	${.CURDIR}/../common
COMMONOBJ!=cd ${.CURDIR}/../common; \
	 printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-

CPPFLAGS+=-I${.CURDIR} \
	-I${COMMON} \
	-I${COMMONOBJ} \
	-I${.CURDIR}/../arch \
	-I${.CURDIR}/../arch/${MACHINE_ARCH} \
	-I${DIST} \
	-I${DIST}/config \
	-I${DIST}/config/${MACHINE_ARCH} \
	-DIN_GCC -DNETBSD_NATIVE

LDADD+=		-lgnumalloc
DPADD+=		${LIBGNUMALLOC}

# Some GNU architecture names don't match NetBSD architecture names.  Most do.
.if ${MACHINE_ARCH} == "arm32"
TARGET_GNU_ARCH=arm
.elif ${MACHINE_ARCH} == "powerpc"
TARGET_GNU_ARCH=rs6000
.else
TARGET_GNU_ARCH=${MACHINE_ARCH}	
.endif

.PATH: ${DIST} ${DIST}/config ${DIST}/config/${TARGET_GNU_ARCH} \
	${COMMON} ${COMMONOBJ}

# XXX
version=egcs-2.90.27
target_alias=${TARGET_GNU_ARCH}-netbsd
CPPFLAGS+= \
	-DDEFAULT_TARGET_VERSION=\"${version}\" \
	-DDEFAULT_TARGET_MACHINE=\"${target_alias}\" \
# XXX

.endif
