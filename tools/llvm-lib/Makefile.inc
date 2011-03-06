#	$NetBSD: Makefile.inc,v 1.1.2.2 2011/03/06 15:07:58 bouyer Exp $

.include "${.PARSEDIR}/../llvm/Makefile.inc"

HOSTLIB=	${LIB}

.include "${LLVM_TOPLEVEL}/lib/Makefile.inc"
.include "${LLVM_TOPLEVEL}/lib/${.CURDIR:T}/Makefile"

HOST_CPPFLAGS:=	${HOST_CPPFLAGS:N-Wp,-iremap,*}
