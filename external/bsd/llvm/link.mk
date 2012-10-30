#	$NetBSD: link.mk,v 1.1.6.1 2012/10/30 18:55:47 yamt Exp $

.include <bsd.own.mk>

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

.if defined(HOSTPROG)
LDADD_NEED_DL=	cat ${LLVM_TOOLCONF_OBJDIR}/need-dl 2> /dev/null
LDADD+=	${LDADD_NEED_DL:sh}
.endif

LDADD+=	-lpthread
