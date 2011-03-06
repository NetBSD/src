#	$NetBSD: Makefile,v 1.1.2.2 2011/03/06 15:07:57 bouyer Exp $

HOSTPROGNAME=	${_TOOL_PREFIX}clang
HOSTPROG_CXX=	${_TOOL_PREFIX}clang
HOST_SRCDIR=	external/bsd/llvm/bin/clang

.include "${.CURDIR}/../Makefile.host"

install.host: ${HOST_BINDIR}/${_TOOL_PREFIX}clang++ ${HOST_BINDIR}/${_TOOL_PREFIX}clang-cpp

${HOST_BINDIR}/${_TOOL_PREFIX}clang++:
	${_MKTARGET_INSTALL}
	mkdir -p ${HOST_BINDIR}
	${HOST_INSTALL_SYMLINK} ${HOSTPROGNAME} ${.TARGET}

${HOST_BINDIR}/${_TOOL_PREFIX}clang-cpp:
	${_MKTARGET_INSTALL}
	mkdir -p ${HOST_BINDIR}
	printf '#!/bin/sh\nexec ${HOST_BINDIR}/${_TOOL_PREFIX}clang -x c -E "$$@"' > ${.TARGET}
	chmod a+x ${.TARGET}
