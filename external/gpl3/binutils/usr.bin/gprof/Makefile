#	$NetBSD: Makefile,v 1.5.12.2 2014/08/19 23:53:05 tls Exp $

.include <bsd.own.mk>

TOP=		${NETBSDSRCDIR}/external/gpl3/binutils

.include "${.CURDIR}/../common/Makefile.inc"
.include "${.CURDIR}/arch/${BINUTILS_MACHINE_ARCH}/defs.mk"

PROG=		gprof
SRCS=		${G_gprof_OBJECTS:.o=.c}
CPPFLAGS+=	-I${.CURDIR}/arch/${BINUTILS_MACHINE_ARCH}

PROGDPLIBS+=	bfd	${TOP}/${BFDSUBDIR}/libbfd
PROGDPLIBS+=	iberty	${TOP}/${BFDSUBDIR}/libiberty

TEXINFO=	${G_TEXINFOS}
COMMONOBJDIR!=	cd ${TOP}/usr.bin/common && ${PRINTOBJDIR}
INFOFLAGS=	-I${COMMONOBJDIR} -I${DIST}/libiberty

gprof.info:	bfdver.texi

.PATH: ${COMMONOBJDIR} ${DIST}/gprof

.include <bsd.prog.mk>
.include <bsd.info.mk>

LDADD+=	-lintl -lz
DPADD+=	${LIBINTL} ${LIBZ}
