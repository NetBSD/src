#	$NetBSD: Makefile,v 1.4 2021/05/30 01:57:00 joerg Exp $

.include <bsd.hostinit.mk>

NO_LLVM_DEVELOPER=	yes

all: depend

.include "${.PARSEDIR}/../llvm/Makefile.inc"

.include "${LLVM_TOPLEVEL}/lib/Makefile"
