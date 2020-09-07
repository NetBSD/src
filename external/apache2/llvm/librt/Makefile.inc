#	$NetBSD: Makefile.inc,v 1.3 2020/09/07 19:05:41 christos Exp $

LLVM_TOPLEVEL:=	${.PARSEDIR}/..
NOSTATICLIB=	yes

COPTS+=${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} > 8:? -Wno-error=init-list-lifetime :}

.include "${LLVM_TOPLEVEL}/lib/Makefile.inc"
.include "${LLVM_TOPLEVEL}/lib/${.CURDIR:T}/Makefile"

CWARNFLAGS.gcc+=	${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} >= 9:? -Wno-error=init-list-lifetime :}
