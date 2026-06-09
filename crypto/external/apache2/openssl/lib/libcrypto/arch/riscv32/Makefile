#	$NetBSD: Makefile,v 1.2 2026/06/09 19:20:32 christos Exp $

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
