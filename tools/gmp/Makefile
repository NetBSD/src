#	$NetBSD: Makefile,v 1.1.4.1 2012/05/07 03:08:35 riz Exp $
#

GNUHOSTDIST=   ${.CURDIR}/../../external/lgpl3/gmp/dist

CONFIGURE_ARGS+=--disable-shared

.include "${.CURDIR}/../Makefile.gnuhost"

# Don't override CFLAGS and CXXFLAGS, it breaks ABI detection.
# PATH needs special treating since "make" tries to expand $PATH
# when variable is used.
CONFIGURE_ENV:=	${CONFIGURE_ENV:NPATH=*:NCFLAGS=*:NCXXFLAGS=*} \
		PATH="${TOOLDIR:Q}/bin:$$$$PATH"

# Force avoiding possibly non-executable install-sh.
CONFIGURE_ENV+=	ac_cv_path_mkdir="${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-install -d"
