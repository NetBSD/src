#	$NetBSD: Makefile,v 1.1 2024/02/23 05:34:41 mrg Exp $

GNUHOSTDIST=	${.CURDIR}/../../external/mit/isl/dist

CONFIGURE_ARGS+=	--with-gmp-prefix=${TOOLDIR:Q} \
			--disable-shared

.include "${.CURDIR}/../Makefile.gmakehost"

CONFIGURE_ENV+= MAKE=${TOOL_GMAKE}

# XXX this gets lost some how.  Not portable.
HOST_CXXFLAGS+=	-std=gnu++17

# Force avoiding possibly non-executable install-sh.
#CONFIGURE_ENV+= ac_cv_path_mkdir="${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-install -d"
