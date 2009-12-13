#	$NetBSD: Makefile,v 1.4 2009/12/13 08:25:20 mrg Exp $

# XXX
.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/compat/Makefile.compat"
depend dependall all: ${DESTDIR}/usr/lib/${MLIBDIR}
depend dependall all: ${DESTDIR}/usr/lib/${MLIBDIR}/i18n
depend dependall all: ${DESTDIR}/usr/lib/${MLIBDIR}/security

.if ${MACHINE_ARCH} == "x86_64"
SUBDIR=	i386_elf
.elif ${MACHINE_ARCH} == "sparc64"
SUBDIR=	sparc_elf
.endif

.include <bsd.subdir.mk>
