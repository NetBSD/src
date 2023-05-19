#	$NetBSD: Makefile,v 1.6 2023/05/19 10:42:34 lukem Exp $

BUILD_ENV= \
		AR=${HOST_AR:Q} \
		AWK=${TOOL_AWK:Q} \
		CC=${HOST_CC:Q} \
		CFLAGS=${HOST_CFLAGS:Q} \
		CONFIG_SHELL=${HOST_SH:Q} \
		CPPFLAGS=${HOST_CPPFLAGS:Q} \
		CXX=${HOST_CXX:Q} \
		CXXFLAGS=${HOST_CXXFLAGS:Q} \
		INSTALL=${HOST_INSTALL_FILE:Q} \
		LDFLAGS=${HOST_LDFLAGS:Q} \
		LEX=${LEX:Q} \
		FLEX=${LEX:Q} \
		M4=${TOOL_M4:Q} \
		MAKE=${TOOL_GMAKE:Q} \
		PATH="${TOOLDIR}/bin:$$PATH" \
		RANLIB=${HOST_RANLIB:Q} \
		YACC=${YACC:Q}

GMAKE_J_ARGS?=	${MAKEFLAGS:[*]:M*-j*:C/(^|.* )(-j ?[0-9][0-9]*).*/\2/W}
BUILD_COMMAND=	/usr/bin/env -i ${BUILD_ENV} ${TOOL_GMAKE} ${GMAKE_J_ARGS} -e

libcpp/libcpp.a:
	[ ! -d libcpp ] && mkdir libcpp || true
	(cd libcpp && ${BUILD_ENV} \
		${HOST_SH} ${DIST}/libcpp/configure -v am_cv_func_iconv=no \
		&& ${BUILD_COMMAND})

cleandir:
	-rm -rf libcpp

depend dependall all: libcpp/libcpp.a

.include <bsd.prog.mk>
