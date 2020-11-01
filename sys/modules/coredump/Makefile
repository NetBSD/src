#	$NetBSD: Makefile,v 1.7 2020/10/19 19:33:02 christos Exp $

.include "../Makefile.inc"

KMOD=	coredump

.PATH:	${S}/kern
SRCS=	kern_core.c core_netbsd.c
SRCS+=	${OBJECT_FMTS:S/^/core_/:S/$/.c/}

.PATH:	${S}/uvm
SRCS+=	uvm_coredump.c

.include "../ptrace_common/machdep.mk"
SRCS+=	core_machdep.c

.include "${.CURDIR}/../../compat/netbsd32/netbsd32.mk"
.if ${COMPAT_USE_NETBSD32} != "no"
CPPFLAGS.core_elf32.c+=     -DCOMPAT_NETBSD32
CPPFLAGS.kern_core.c+=     -DCOMPAT_NETBSD32
.endif

WARNS=	3

COPTS.kern_core.c+=	${GCC_NO_CAST_FUNCTION_TYPE}

.include <bsd.kmodule.mk>
