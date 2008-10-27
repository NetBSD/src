#	$NetBSD: Makefile,v 1.2 2008/10/27 07:18:44 mrg Exp $

# XXX
.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/compat/Makefile.compat"
depend dependall all: ${DESTDIR}/usr/lib/${LD32DIR}
depend dependall all: ${DESTDIR}/usr/lib/${LD32DIR}/i18n

.if ${MACHINE_ARCH} == "x86_64"
SUBDIR=	i386_elf
.elif ${MACHINE_ARCH} == "sparc64"
SUBDIR=	sparc_elf
.endif

.include <bsd.subdir.mk>
