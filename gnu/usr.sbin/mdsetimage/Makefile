#	$NetBSD: Makefile,v 1.16 2003/11/27 14:36:45 mrg Exp $

.include <bsd.own.mk>

.if (defined(HOSTPROG) && \
     (${TOOLCHAIN_MISSING} != "yes" || defined(EXTERNAL_TOOLCHAIN))) || \
    ${MKBFD} != "no"
TOP=		${NETBSDSRCDIR}/gnu
.if ${HAVE_BINUTILS214} == "yes"
DIST=		${TOP}/dist/binutils
BFDSUBDIR=	usr.bin/binutils
.else
DIST=		${.CURDIR}/../../../dist/toolchain
DIST=		${TOP}/dist/toolchain
BFDSUBDIR=	lib
.endif

PROG=		mdsetimage
MAN=		mdsetimage.8

CPPFLAGS+=	-I${BFDDIR} -I${DIST}/bfd -I${DIST}/binutils -I${DIST}/include

# XXX GCC3/m68010 fails to compile mdsetimage.c
.if ${MACHINE_ARCH} == "m68000"
COPTS.mdsetimage.c=	-O0
.endif

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
