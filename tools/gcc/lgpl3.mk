# $NetBSD: lgpl3.mk,v 1.1 2023/08/09 18:57:04 christos Exp $
# configuration for native build to find lgpl3 libraries
#
.include "../../external/lgpl3/gmp/Makefile.arch"

MPC=		${NETBSDSRCDIR}/external/lgpl3/mpc
MPFR=		${NETBSDSRCDIR}/external/lgpl3/mpfr
GMP=		${NETBSDSRCDIR}/external/lgpl3/gmp

MPCOBJ!=	cd ${MPC}/lib/libmpc && ${PRINTOBJDIR}
MPFROBJ!=	cd ${MPFR}/lib/libmpfr && ${PRINTOBJDIR}
GMPOBJ!=	cd ${GMP}/lib/libgmp && ${PRINTOBJDIR}

MPCINC=		${MPC}/dist/src
MPFRINC=	${MPFR}/dist/src
GMPINC=		${GMP}/lib/libgmp/arch/${GMP_MACHINE_ARCH}

NATIVE_CONFIGURE_ARGS+=	\
			--with-mpc-lib=${MPCOBJ} \
			--with-mpfr-lib=${MPFROBJ} \
			--with-gmp-lib=${GMPOBJ} \
			--with-mpc-include=${MPCINC} \
			--with-mpfr-include=${MPFRINC} \
			--with-gmp-include=${GMPINC}
