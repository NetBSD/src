#	$NetBSD: Makefile,v 1.5 2003/03/01 13:09:06 mrg Exp $

.include <bsd.own.mk>

TOP=		${NETBSDSRCDIR}/gnu
DIST=		${TOP}/dist/toolchain

.include "${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk"

PROG=		as
SRCS=		${G_OBJS:Nitbl-[pl]*:Nm68k-parse.o:.o=.c} \
		${G_OBJS:Mitbl-parse.o:.o=.y} \
		${G_OBJS:Mitbl-lex.o:.o=.l} \
		${G_OBJS:Mm68k-parse.o:.o=.y}

CPPFLAGS+=	-I${.CURDIR}/arch/${MACHINE_ARCH}
CPPFLAGS+=	-I${DIST}/gas -I${DIST}/gas/config
CPPFLAGS+=	-I${DIST}

LDADD=		-lintl
DPADD=		${LIBINTL}

OPCODESOBJ!=	cd ${TOP}/lib/libopcodes && ${PRINTOBJDIR}
LDADD+=		-L${OPCODESOBJ} -lopcodes
DPADD+=		${OPCODESOBJ}/libopcodes_pic.a

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
