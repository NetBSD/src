#	$NetBSD: Makefile,v 1.3.8.1 2009/09/13 21:21:16 matt Exp $

# XXX
.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/compat/Makefile.compat"
depend dependall all: ${DESTDIR}/usr/lib/${LD32DIR}
depend dependall all: ${DESTDIR}/usr/lib/${LD32DIR}/i18n
depend dependall all: ${DESTDIR}/usr/lib/${LD32DIR}/security

.if ${MACHINE_ARCH} == "x86_64"
SUBDIR=	i386_elf
.elif ${MACHINE_ARCH} == "sparc64"
SUBDIR=	sparc_elf
.elif ${MACHINE_ARCH} == "mips64eb" || ${MACHINE_ARCH} == "mips64el"
SUBDIR=	mips
.endif

.include <bsd.subdir.mk>
