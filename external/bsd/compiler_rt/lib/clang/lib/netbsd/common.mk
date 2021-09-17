# $NetBSD: common.mk,v 1.6 2021/09/17 02:18:01 christos Exp $

NOSANITIZER=	# defined
NODEBUG=	# defined
NOLIBCSANITIZER=# defined

.include <bsd.own.mk>

TOPDIR= ${NETBSDSRCDIR}/sys/external/bsd/compiler_rt/dist

CLANG_VER!=	cd ${NETBSDSRCDIR}/external/apache2/llvm && \
    ${MAKE} -V LLVM_VERSION
