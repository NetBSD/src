#	$NetBSD: Makefile,v 1.1 2017/02/09 17:58:57 joerg Exp $

PROG_CXX=	llvm-xray
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-xray

SRCS=	func-id-helper.cc \
	xray-account.cc \
	xray-converter.cc

LLVM_LIBS+= \
	Support \
	Demangle

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
