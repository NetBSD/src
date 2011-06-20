#	$NetBSD: Makefile,v 1.1 2011/06/20 05:56:46 mrg Exp $
#

GNUHOSTDIST=   ${.CURDIR}/../../external/lgpl3/gmp/dist

CONFIGURE_ARGS+=--disable-shared

.include "${.CURDIR}/../Makefile.gnuhost"

# Don't override CFLAGS and CXXFLAGS, it breaks ABI detection.
# PATH needs special treating since "make" tries to expand $PATH
# when variable is used.
CONFIGURE_ENV:=	${CONFIGURE_ENV:NPATH=*:NCFLAGS=*:NCXXFLAGS=*} \
		PATH="${TOOLDIR:Q}/bin:$$$$PATH"
