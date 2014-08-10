#	$NetBSD: Makefile,v 1.39.2.1 2014/08/10 06:54:05 tls Exp $

.if !defined(INCSDIR)
INCSDIR?= /usr/include/powerpc
INCS+=	spr.h
.endif

INCS+=	ansi.h aout_machdep.h asm.h \
	bswap.h \
	cdefs.h cpu.h \
	elf_machdep.h endian.h endian_machdep.h \
	float.h fpu.h frame.h \
	ieee.h ieeefp.h \
	int_const.h int_fmtio.h int_limits.h int_mwgwtypes.h int_types.h \
	kcore.h \
	limits.h lock.h \
	math.h mcontext.h mutex.h \
	param.h pcb.h pmap.h pmc.h proc.h profile.h psl.h pte.h ptrace.h \
	reg.h reloc.h rwlock.h \
	setjmp.h signal.h sljit_machdep.h \
	trap.h types.h \
	vmparam.h \
	wchar_limits.h

.include <bsd.kinc.mk>
