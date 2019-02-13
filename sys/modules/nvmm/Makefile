#	$NetBSD: Makefile,v 1.2 2019/02/13 16:03:16 maxv Exp $

.include "../Makefile.inc"
.include "../Makefile.assym"

CPPFLAGS+=

.PATH:	${S}/dev/nvmm
.PATH:	${S}/dev/nvmm/x86

KMOD=	nvmm
IOCONF=	nvmm.ioconf
SRCS=	nvmm.c

.if ${MACHINE_ARCH} == "x86_64"
SRCS+=	nvmm_x86_svm.c nvmm_x86_svmfunc.S
SRCS+=	nvmm_x86_vmx.c nvmm_x86_vmxfunc.S
.endif

.include <bsd.kmodule.mk>
