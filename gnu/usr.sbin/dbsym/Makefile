#	$NetBSD: Makefile,v 1.7 2002/09/19 03:09:43 lukem Exp $

.include <bsd.own.mk>

.if (defined(HOSTPROG) && \
     (${TOOLCHAIN_MISSING} != "yes" || defined(EXTERNAL_TOOLCHAIN))) || \
    ${MKBFD} != "no"
TOP=		${NETBSDSRCDIR}/gnu
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

BFDDIR!=	cd ${TOP}/lib/libbfd && ${PRINTOBJDIR}
IBERTYDIR!=	cd ${TOP}/lib/libiberty && ${PRINTOBJDIR}

CPPFLAGS+=	-I${TOP}/lib/libbfd/arch/${MACHINE_ARCH} ${G_INCLUDES}
LDADD+=		-lintl
DPADD+=		${LIBINTL}
.endif # HOSTPROG
.endif # MKBFD != no

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif # HOSTPROG
