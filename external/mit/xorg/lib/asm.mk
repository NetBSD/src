#	$NetBSD: asm.mk,v 1.2 2019/04/05 10:31:53 maya Exp $

# Shared with libmesa.mk / libGL / libglapi

.if ${MACHINE} == "amd64"
CPPFLAGS+=	\
	-DUSE_SSE41 \
	-DUSE_X86_64_ASM
.elif ${MACHINE} == "i386"
CPPFLAGS+=	\
	-DUSE_X86_ASM \
	-DUSE_MMX_ASM \
	-DUSE_3DNOW_ASM \
	-DUSE_SSE_ASM \
	-DGLX_X86_READONLY_TEXT
.elif ${MACHINE} == "sparc" || ${MACHINE} == "sparc64"
.endif
