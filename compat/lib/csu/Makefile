#	$NetBSD: Makefile,v 1.3 2008/10/27 07:42:55 mrg Exp $

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
.endif

.include <bsd.subdir.mk>
