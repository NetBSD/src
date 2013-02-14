#	$NetBSD: Makefile,v 1.4 2013/02/14 01:21:26 matt Exp $

.include <bsd.own.mk>

.if (defined(HOSTPROG) && \
     (${TOOLCHAIN_MISSING} == "no" || defined(EXTERNAL_TOOLCHAIN))) || \
    ${MKBINUTILS} != "no"
TOP=		${NETBSDSRCDIR}/external/gpl3/binutils
DIST=		${TOP}/dist
BFDSUBDIR=	lib

PROG=		dbsym
MAN=		dbsym.8

CPPFLAGS+=	-I${BFDDIR} -I${DIST}/bfd -I${DIST}/binutils -I${DIST}/include

.if !defined(HOSTPROG)
LIBDPLIBS+=	-lbfd ${BFDDIR}
.else
DPADD+=		${BFDDIR}/libbfd.a
.endif
LDADD+=		-L${BFDDIR} -lbfd

DPADD+=		${IBERTYDIR}/libiberty.a
LDADD+=		-L${IBERTYDIR} -liberty

.ifndef HOSTPROG
.include	"${TOP}/${BFDSUBDIR}/libbfd/arch/${MACHINE_ARCH}/defs.mk"

BFDDIR!=	cd ${TOP}/${BFDSUBDIR}/libbfd && ${PRINTOBJDIR}
IBERTYDIR!=	cd ${TOP}/${BFDSUBDIR}/libiberty && ${PRINTOBJDIR}

CPPFLAGS+=	-I${TOP}/${BFDSUBDIR}/libbfd/arch/${MACHINE_ARCH} ${G_INCLUDES}
LDADD+=		-lintl
DPADD+=		${LIBINTL}
.endif # HOSTPROG
.endif # MKBINUTILS != no

.include <bsd.prog.mk>
