#	$NetBSD: Makefile,v 1.2.2.1 2014/08/20 00:04:56 tls Exp $
#

GNUHOSTDIST=	${.CURDIR}/../../external/lgpl3/mpc/dist

CONFIGURE_ARGS+=	--with-gmp=${TOOLDIR:Q} \
			--with-mpfr=${TOOLDIR:Q}

CONFIGURE_ARGS+=--disable-shared

.include "${.CURDIR}/../Makefile.gnuhost"

# Force avoiding possibly non-executable install-sh.
CONFIGURE_ENV+= ac_cv_path_mkdir="${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-install -d"
