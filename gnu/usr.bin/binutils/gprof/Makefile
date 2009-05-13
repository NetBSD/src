#	$NetBSD: Makefile,v 1.5.40.1 2009/05/13 19:18:17 jym Exp $

.include <bsd.own.mk>

TOP=		${NETBSDSRCDIR}/gnu

.include "${.CURDIR}/../common/Makefile.inc"
.include "${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk"

PROG=		gprof
SRCS=		${G_gprof_OBJECTS:.o=.c}
CPPFLAGS+=	-I${.CURDIR}/arch/${MACHINE_ARCH} -DYY_NO_INPUT

LDADD=		-lintl
DPADD=		${LIBINTL}

BFDOBJ!=	cd ${TOP}/${BFDSUBDIR}/libbfd && ${PRINTOBJDIR}
LDADD+=		-L${BFDOBJ} -lbfd
.if ${MKPICLIB} != "no"
DPADD+=		${BFDOBJ}/libbfd_pic.a
.else
DPADD+=		${BFDOBJ}/libbfd.a
.endif

IBERTYOBJ!=	cd ${TOP}/${BFDSUBDIR}/libiberty && ${PRINTOBJDIR}
LDADD+=		-L${IBERTYOBJ} -liberty
DPADD+=		${IBERTYOBJ}/libiberty.a

TEXINFO=	${G_TEXINFOS}

.PATH: ${DIST}/gprof

.include <bsd.prog.mk>
.include <bsd.info.mk>
