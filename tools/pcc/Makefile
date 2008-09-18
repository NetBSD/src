# $NetBSD: Makefile,v 1.1.2.2 2008/09/18 04:37:35 wrstuden Exp $

GNUHOSTDIST=	${.CURDIR}/../../external/bsd/pcc/dist/pcc

CONFIGURE_ENV+= CC=${HOST_CC:Q} \
                CFLAGS=${HOST_CFLAGS:Q} \
                CPPFLAGS=${HOST_CPPFLAGS:Q} \
                CXX=${HOST_CXX:Q}

CONFIGURE_ARGS=		--target=${MACHINE_GNU_PLATFORM}
CONFIGURE_ARGS+=	--with-assembler=${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-as
CONFIGURE_ARGS+=	--with-linker=${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-ld

.NOTPARALLEL:

.include "${.CURDIR}/../Makefile.gnuhost"
