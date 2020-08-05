#	$NetBSD: Makefile,v 1.5 2020/08/05 10:33:01 maxv Exp $

.include "../Makefile.inc"
.include "../Makefile.assym"

CPPFLAGS+=

.PATH:	${S}/dev/nvmm
.PATH:	${S}/dev/nvmm/x86

KMOD=	nvmm
IOCONF=	nvmm.ioconf
SRCS=	nvmm.c

.if ${MACHINE_ARCH} == "x86_64"
SRCS+=	nvmm_x86.c
SRCS+=	nvmm_x86_svm.c nvmm_x86_svmfunc.S
SRCS+=	nvmm_x86_vmx.c nvmm_x86_vmxfunc.S
.endif

WARNS=	5

.include <bsd.kmodule.mk>
