#	$NetBSD: link.mk,v 1.2 2021/05/30 01:56:45 joerg Exp $

.include <bsd.own.mk>

LLVM_TOPLEVEL:=	${.PARSEDIR}

.if defined(HOSTPROG)
LIB_BASE=	${NETBSDSRCDIR}/tools/llvm-lib
.else
LIB_BASE=	${LLVM_TOPLEVEL}/lib
.endif

.for l in ${CLANG_LIBS}
CLANG_OBJDIR.${l}!=	cd ${LIB_BASE}/lib${l} && ${PRINTOBJDIR}
LDADD+=	-L${CLANG_OBJDIR.${l}} -l${l}
DPADD+=	${CLANG_OBJDIR.${l}}/lib${l}.a
.endfor

.for l in ${LLVM_LIBS}
LLVM_OBJDIR.${l}!=	cd ${LIB_BASE}/libLLVM${l} && ${PRINTOBJDIR}
LDADD+=	-L${LLVM_OBJDIR.${l}} -lLLVM${l}
DPADD+=	${LLVM_OBJDIR.${l}}/libLLVM${l}.a
.endfor

.for l in ${LLVMRT_LIBS}
LLVMRT_OBJDIR.${l}!=	cd ${LLVM_TOPLEVEL}/librt/libLLVM${l} && ${PRINTOBJDIR}
LDADD+=	${LLVMRT_OBJDIR.${l}}/libLLVM${l}_pic.a
DPADD+=	${LLVMRT_OBJDIR.${l}}/libLLVM${l}_pic.a
.endfor

.if defined(HOSTPROG)
LDADD_NEED_DL=		cat ${LLVM_TOOLCONF_OBJDIR}/need-dl 2> /dev/null || true
LDADD_NEED_TERMINFO=	cat ${LLVM_TOOLCONF_OBJDIR}/need-terminfo 2> /dev/null || true
LDADD+=	${LDADD_NEED_DL:sh} ${LDADD_NEED_TERMINFO:sh}
.else
LDADD+=	-lterminfo
DPADD+=	${LIBTERMINFO}
.endif

LDADD+=	-lpthread
