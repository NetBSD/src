#	$NetBSD: Makefile,v 1.1.4.1 2021/05/31 22:06:59 cjep Exp $

PROG_CXX=	llvm-profdata
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-profdata

SRCS=	llvm-profdata.cpp

LLVM_LIBS+= \
	ProfileData \
	IR \
	Remarks \
	BinaryFormat \
	Support \
	Demangle

.include "${.PARSEDIR}/../../link.mk"

LDADD+=	-lz
.if !defined(HOSTPROG)
DPADD+=	${LIBZ}
.endif

.include <bsd.prog.mk>
