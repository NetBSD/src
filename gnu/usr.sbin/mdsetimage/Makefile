#	$NetBSD: Makefile,v 1.15 2003/10/28 08:21:26 mrg Exp $

.include <bsd.own.mk>

.if (defined(HOSTPROG) && \
     (${TOOLCHAIN_MISSING} != "yes" || defined(EXTERNAL_TOOLCHAIN))) || \
    ${MKBFD} != "no"
TOP=		${NETBSDSRCDIR}/gnu
DIST=		${TOP}/dist/toolchain

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
.include	"${TOP}/lib/libbfd/arch/${MACHINE_ARCH}/defs.mk"

BFDDIR!=	cd ${TOP}/lib/libbfd && ${PRINTOBJDIR}
IBERTYDIR!=	cd ${TOP}/lib/libiberty && ${PRINTOBJDIR}

CPPFLAGS+=	-I${TOP}/lib/libbfd/arch/${MACHINE_ARCH} ${G_INCLUDES}
LDADD+=		-lintl
DPADD+=		${LIBINTL}
.endif # HOSTPROG
.endif # MKBFD != no

.include <bsd.prog.mk>
