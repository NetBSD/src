#	$NetBSD: Makefile,v 1.1 2006/05/15 16:31:37 nathanw Exp $

NOMAN=	# defined

.include <bsd.own.mk>

TOP=	${NETBSDSRCDIR}/gnu
DIST=	${TOP}/dist/gdb6

PROG=	psim
SRCS=	main.c
CPPFLAGS+=	-I${SIMDIR} -I${.CURDIR}/../../arch/powerpc

.include <bsd.prog.mk>
.include "../../../Makefile.inc"

SIMDIR!=	cd ${.CURDIR}/../ppc && ${PRINTOBJDIR}

LIBBFDDIR!=	cd ${.CURDIR}/../../bfd && ${PRINTOBJDIR}
LDADD+=		-L${LIBBFDDIR} -lbfd
DPADD+=		${LIBBFDDIR}/libbfd.a

LIBIBERTYDIR!=	cd ${.CURDIR}/../../libiberty && ${PRINTOBJDIR}
LDADD+=		-L${LIBIBERTYDIR} -liberty
DPADD+=		${LIBIBERTYDIR}/libiberty.a

LDADD+=		-L${SIMDIR} -lsim
DPADD+=		${SIMDIR}/libsim.a

.PATH: ${DIST}/sim/ppc 
