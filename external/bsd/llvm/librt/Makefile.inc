#	$NetBSD: Makefile.inc,v 1.3.2.2 2019/06/10 21:50:58 christos Exp $

LLVM_TOPLEVEL:=	${.PARSEDIR}/..
NOSTATICLIB=	yes

.include "${LLVM_TOPLEVEL}/lib/Makefile.inc"
.include "${LLVM_TOPLEVEL}/lib/${.CURDIR:T}/Makefile"
