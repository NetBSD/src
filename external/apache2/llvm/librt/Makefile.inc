#	$NetBSD: Makefile.inc,v 1.3.2.1 2021/05/31 22:10:14 cjep Exp $

LLVM_TOPLEVEL:=	${.PARSEDIR}/..
NOSTATICLIB=	yes

CWARNFLAGS.gcc+=	${${HAVE_GCC:U0} >= 9:? -Wno-error=init-list-lifetime :}

.include "${LLVM_TOPLEVEL}/lib/Makefile.inc"
.include "${LLVM_TOPLEVEL}/lib/${.CURDIR:T}/Makefile"

LLVM_INCLUDE_OBJDIR!=	cd ${LLVM_TOPLEVEL}/include && ${PRINTOBJDIR}
LLVM_MODULE_CACHE=	${LLVM_INCLUDE_OBJDIR}/module.cache-rt
