#	$NetBSD: Makefile,v 1.1.2.1 2026/07/04 15:29:32 martin Exp $

.include "bsd.own.mk"

CRYPTODIST=${NETBSDSRCDIR}/crypto
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"

.if make(regen) && ${HAVE_LLVM:U} == "yes"
CC+= -fno-integrated-as
.endif

regen:
	for i in $$(find ${OPENSSLSRC} -name \*riscv32\*.pl); do \
		j=$$(basename $$i .pl).S; \
		(echo "#include <machine/asm.h>"; CC=${CC:Q} perl $$i) > $$j; \
	done
