#	$NetBSD: Makefile,v 1.1 2001/08/06 19:12:02 tv Exp $

TOP=		${.CURDIR}/../../..
DIST=		${TOP}/dist/toolchain

.include <bsd.own.mk>
.include "${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk"

PROG=		as
SRCS=		${G_OBJS:.o=.c}

CPPFLAGS+=	-I${.CURDIR}/arch/${MACHINE_ARCH}

LDADD=		-lintl
DPADD=		${LIBINTL}

BFDOBJ!=	cd ${TOP}/lib/libbfd && ${PRINTOBJDIR}
LDADD+=		-L${BFDOBJ} -lbfd
DPADD+=		${BFDOBJ}/libbfd_pic.a

IBERTYOBJ!=	cd ${TOP}/lib/libiberty && ${PRINTOBJDIR}
LDADD+=		-L${IBERTYOBJ} -liberty
DPADD+=		${IBERTYOBJ}/libiberty.a

TEXINFO=	as.texinfo
INFOFLAGS=	-I${DIST}/gas/doc -I${.CURDIR}

.PATH: ${DIST}/gas ${DIST}/gas/config ${DIST}/gas/doc

.include <bsd.prog.mk>
.include <bsd.info.mk>
