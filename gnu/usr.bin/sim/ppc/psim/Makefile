#	$NetBSD: Makefile,v 1.1 2002/01/22 16:55:20 mrg Exp $

TOP=	${.CURDIR}/../../../..

PROG=	psim
SRCS=	main.c
CPPFLAGS+=	-I${SIMDIR} -I${.CURDIR}/../../arch/powerpc
NOMAN=

LDADD+=	-lintl
DPADD+=	${LIBINTL}

.include <bsd.prog.mk>

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
