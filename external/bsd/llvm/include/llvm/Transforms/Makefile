#	$NetBSD: Makefile,v 1.1 2018/12/31 19:33:05 maya Exp $

.include <bsd.init.mk>

.PATH:	${LLVM_SRCDIR}/include/llvm/Transforms

SUBDIR+=	IPO InstCombine Instrumentation Scalar Utils Vectorize

INCS=		Coroutines.h IPO.h Instrumentation.h ObjCARC.h \
		Scalar.h Utils.h Vectorize.h 

INCSDIR=	/usr/include/llvm/Transforms

.include <bsd.subdir.mk>
.include <bsd.prog.mk>
