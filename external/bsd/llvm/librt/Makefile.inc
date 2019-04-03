#	$NetBSD: Makefile.inc,v 1.3 2019/04/03 15:21:58 joerg Exp $

LLVM_TOPLEVEL:=	${.PARSEDIR}/..
NOSTATICLIB=	yes

.include "${LLVM_TOPLEVEL}/lib/Makefile.inc"
.include "${LLVM_TOPLEVEL}/lib/${.CURDIR:T}/Makefile"
