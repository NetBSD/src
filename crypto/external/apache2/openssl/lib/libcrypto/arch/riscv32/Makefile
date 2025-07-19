#	$NetBSD: Makefile,v 1.1 2025/07/19 17:41:37 christos Exp $

.include "bsd.own.mk"

CRYPTODIST=${NETBSDSRCDIR}/crypto
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"

.if make(regen) && ${HAVE_LLVM:U} == "yes"
CC+= -fno-integrated-as
.endif

regen:
	for i in $$(find ${OPENSSLSRC} -name \*${MACHINE_ARCH}\*.pl); do \
		j=$$(basename $$i .pl).S; \
		(echo "#include <machine/asm.h>"; CC=${CC:Q} perl $$i) > $$j; \
	done
