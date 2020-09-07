#	$NetBSD: Makefile.inc,v 1.2 2020/09/07 00:18:26 mrg Exp $

LLVM_TOPLEVEL:=	${.PARSEDIR}/..
NOSTATICLIB=	yes

.include "${LLVM_TOPLEVEL}/lib/Makefile.inc"
.include "${LLVM_TOPLEVEL}/lib/${.CURDIR:T}/Makefile"

CWARNFLAGS.gcc+=	${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} >= 9:? -Wno-error=init-list-lifetime :}
