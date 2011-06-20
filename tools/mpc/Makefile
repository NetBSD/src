#	$NetBSD: Makefile,v 1.1 2011/06/20 05:56:46 mrg Exp $
#

GNUHOSTDIST=	${.CURDIR}/../../external/lgpl2/mpc/dist

CONFIGURE_ARGS+=	--with-gmp=${TOOLDIR:Q} \
			--with-mpfr=${TOOLDIR:Q}

CONFIGURE_ARGS+=--disable-shared

.include "${.CURDIR}/../Makefile.gnuhost"
