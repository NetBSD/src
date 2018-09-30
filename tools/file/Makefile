#	$NetBSD: Makefile,v 1.7.30.1 2018/09/30 01:45:59 pgoyette Exp $
#

GNUHOSTDIST=	${.CURDIR}/../../external/bsd/file/dist

CONFIGURE_ENV+=	CC=${HOST_CC:Q} \
		CFLAGS=${HOST_CFLAGS:Q} \
		CPPFLAGS=${HOST_CPPFLAGS:Q} \
		CXX=${HOST_CXX:Q}

CONFIGURE_ARGS=	--program-prefix=${_TOOL_PREFIX} --disable-shared \
		--disable-warnings --disable-libseccomp

.NOTPARALLEL:

.include "${.CURDIR}/../Makefile.gnuhost"

# Force avoiding possibly non-executable install-sh.
CONFIGURE_ENV+= ac_cv_path_mkdir="${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-install -d"
