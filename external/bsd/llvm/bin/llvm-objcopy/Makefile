#	$NetBSD: Makefile,v 1.1 2018/07/17 18:55:26 joerg Exp $

PROG_CXX=	llvm-objcopy
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-objcopy

SRCS=	Object.cpp \
	llvm-objcopy.cpp

TABLEGEN_SRC=		ObjcopyOpts.td StripOpts.td
TABLEGEN_OUTPUT.ObjcopyOpts.td= \
	ObjcopyOpts.inc|-gen-opt-parser-defs

TABLEGEN_OUTPUT.StripOpts.td= \
	StripOpts.inc|-gen-opt-parser-defs
.include "${.PARSEDIR}/../../tablegen.mk"

LLVM_LIBS+= \

LDADD+=	-lz
DPADD+=	${LIBZ}

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
