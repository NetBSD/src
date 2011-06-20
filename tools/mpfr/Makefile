#	$NetBSD: Makefile,v 1.1 2011/06/20 05:56:45 mrg Exp $
#

GNUHOSTDIST=	${.CURDIR}/../../external/lgpl3/mpfr/dist

CONFIGURE_ARGS+=	--with-gmp=${TOOLDIR:Q}

CONFIGURE_ARGS+=--disable-shared

.include "${.CURDIR}/../Makefile.gnuhost"
