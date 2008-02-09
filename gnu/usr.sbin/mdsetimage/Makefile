#	$NetBSD: Makefile,v 1.19 2008/02/09 02:37:21 mrg Exp $

.include <bsd.own.mk>

.if (defined(HOSTPROG) && \
     (${TOOLCHAIN_MISSING} == "no" || defined(EXTERNAL_TOOLCHAIN))) || \
    ${MKBFD} != "no"
TOP=		${NETBSDSRCDIR}/gnu
DIST=		${TOP}/dist/binutils
BFDSUBDIR=	lib

PROG=		mdsetimage
MAN=		mdsetimage.8

CPPFLAGS+=	-I${BFDDIR} -I${DIST}/bfd -I${DIST}/binutils -I${DIST}/include

.if !defined(HOSTPROG) && ${MKPICLIB} != "no"
DPADD+=		${BFDDIR}/libbfd_pic.a
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
.endif # MKBFD != no

.include <bsd.prog.mk>
