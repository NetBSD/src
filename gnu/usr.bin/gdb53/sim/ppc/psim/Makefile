#	$NetBSD: Makefile,v 1.1.1.1 2004/04/02 13:06:01 mrg Exp $

NOMAN=	# defined

.include <bsd.own.mk>

TOP=	${NETBSDSRCDIR}/gnu
DIST=	${TOP}/dist/toolchain

PROG=	psim
SRCS=	main.c
CPPFLAGS+=	-I${SIMDIR} -I${.CURDIR}/../../arch/powerpc

LDADD+=	-lintl
DPADD+=	${LIBINTL}

.include <bsd.prog.mk>
.include "../../../Makefile.inc"

SIMDIR!=	cd ${TOP}/usr.bin/sim/ppc && ${PRINTOBJDIR}

BFDOBJ!=	cd ${TOP}/lib/libbfd && ${PRINTOBJDIR}
LDADD+=		-L${BFDOBJ} -lbfd
DPADD+=		${BFDOBJ}/libbfd_pic.a

IBERTYOBJ!=	cd ${TOP}/lib/libiberty && ${PRINTOBJDIR}
LDADD+=		-L${IBERTYOBJ} -liberty
DPADD+=		${IBERTYOBJ}/libiberty.a

PSIMOBJ!=	cd ${TOP}/usr.bin/sim/ppc && ${PRINTOBJDIR}
LDADD+=		-L${PSIMOBJ} -lsim
DPADD+=		${PSIMOBJ}/libsim.a

.PATH: ${DIST}/sim/ppc 
