#	$NetBSD: Makefile,v 1.6 2002/09/17 23:18:31 thorpej Exp $

.include <bsd.own.mk>

.if (defined(HOSTPROG) && \
     (${TOOLCHAIN_MISSING} != "yes" || defined(EXTERNAL_TOOLCHAIN))) || \
    ${MKBFD} != "no"
TOP=		${.CURDIR}/../..
DIST=		${TOP}/dist/toolchain

PROG=		dbsym
MAN=		dbsym.8

CPPFLAGS+=	-I${BFDDIR} -I${DIST}/bfd -I${DIST}/binutils -I${DIST}/include

DPADD+=		${BFDDIR}/libbfd.a
LDADD+=		-L${BFDDIR} -lbfd

DPADD+=		${IBERTYDIR}/libiberty.a
LDADD+=		-L${IBERTYDIR} -liberty

.ifndef HOSTPROG
.include	"${TOP}/lib/libbfd/arch/${MACHINE_ARCH}/defs.mk"

BFDDIR!=	cd ${.CURDIR}/../../lib/libbfd && ${PRINTOBJDIR}
IBERTYDIR!=	cd ${TOP}/lib/libiberty && ${PRINTOBJDIR}

CPPFLAGS+=	-I${TOP}/lib/libbfd/arch/${MACHINE_ARCH} ${G_INCLUDES}
LDADD+=		-lintl
DPADD+=		${LIBINTL}
.endif # HOSTPROG
.endif # MKBFD != no

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif # HOSTPROG
