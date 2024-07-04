# $NetBSD: bfd.mk,v 1.1 2024/07/04 02:20:03 christos Exp $

BFDSUBDIR=      lib

CPPFLAGS+=	-I${DIST}/bfd -I${DIST}/binutils -I${DIST}/include

.if !defined(HOSTPROG)
PROGDPLIBS+=	bfd	${TOP}/${BFDSUBDIR}/libbfd
PROGDPLIBS+=	sframe	${TOP}/${BFDSUBDIR}/libsframe
PROGDPLIBS+=	iberty	${TOP}/${BFDSUBDIR}/libiberty
.else
CPPFLAGS+=	-I${BFDDIR}
DPADD+=		${BFDDIR}/.libs/libbfd.a
LDADD+=		-L${BFDDIR}/.libs -lbfd
DPADD+=		${SFRAMEDIR}/.libs/libsframe.a
LDADD+=		-L${SFRAMEDIR}/.libs -lsframe
DPADD+=		${IBERTYDIR}/libiberty.a
LDADD+=		-L${IBERTYDIR} -liberty
.endif

.include <bsd.prog.mk>

.ifndef HOSTPROG
.include	"${TOP}/${BFDSUBDIR}/libbfd/arch/${BINUTILS_MACHINE_ARCH}/defs.mk"
CPPFLAGS+=	-I${TOP}/${BFDSUBDIR}/libbfd/arch/${BINUTILS_MACHINE_ARCH} ${G_INCLUDES}
LDADD+=		-lintl
DPADD+=		${LIBINTL}
.endif # HOSTPROG

LDADD+=         -lz
# No DPADD because we don't know the format and we didn't build it.
