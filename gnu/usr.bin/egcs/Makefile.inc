#	$NetBSD: Makefile.inc,v 1.8 1998/08/17 02:25:44 tv Exp $

.if !defined(GCC_MAKEFILE_INC)
GCC_MAKEFILE_INC=1

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../../dist/gcc
COMMON=		${.CURDIR}/../common
COMMONOBJ!=	cd ${.CURDIR}/../common; \
	 	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-

CPPFLAGS+=	-I${.CURDIR}/../arch \
		-I${.CURDIR}/../arch/${MACHINE_ARCH} \
		-I${COMMONOBJ} \
		-I${DIST} \
		-I${DIST}/config \
		-DIN_GCC -DNETBSD_NATIVE

LDADD+=		-lgnumalloc
DPADD+=		${LIBGNUMALLOC}

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
