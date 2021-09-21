#	$NetBSD: Makefile.inc,v 1.1 2021/09/21 20:34:37 christos Exp $

TOPDIR:=	${.PARSEDIR}/../../../sys/external/bsd/compiler_rt/dist
COMPILER_RT:=	${.PARSEDIR}

CLANG_VER!=    cd ${COMPILER_RT}/../../apache2/llvm && ${MAKE} -V LLVM_VERSION
