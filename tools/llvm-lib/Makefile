#	$NetBSD: Makefile,v 1.3.8.1 2021/05/31 22:15:24 cjep Exp $

.include <bsd.hostinit.mk>

NO_LLVM_DEVELOPER=	yes

all: depend

.include "${.PARSEDIR}/../llvm/Makefile.inc"

.include "${LLVM_TOPLEVEL}/lib/Makefile"
