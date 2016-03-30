# $NetBSD: Makefile,v 1.3 2016/03/30 23:36:45 riastradh Exp $

GNUHOSTDIST=	${.CURDIR}/../../external/bsd/pcc/dist/pcc

CONFIGURE_ENV+= CC=${HOST_CC:Q} \
                CFLAGS=${HOST_CFLAGS:Q} \
                CPPFLAGS=${HOST_CPPFLAGS:Q} \
                CXX=${HOST_CXX:Q}

CONFIGURE_ARGS=		--target=${MACHINE_GNU_PLATFORM}
CONFIGURE_ARGS+=	--disable-stripping
CONFIGURE_ARGS+=	--enable-tls
CONFIGURE_ARGS+=	--includedir=/usr/include
CONFIGURE_ARGS+=	--with-incdir=/usr/include
CONFIGURE_ARGS+=	--with-libdir=/usr/lib
CONFIGURE_ARGS+=	--with-assembler=${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-as
CONFIGURE_ARGS+=	--with-linker=${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-ld

.include "${.CURDIR}/../Makefile.gnuhost"
