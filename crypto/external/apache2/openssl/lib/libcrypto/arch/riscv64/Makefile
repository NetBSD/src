#	$NetBSD: Makefile,v 1.2 2025/07/19 20:02:17 christos Exp $

.include "bsd.own.mk"

CRYPTODIST=${NETBSDSRCDIR}/crypto
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"

.if make(regen) && ${HAVE_LLVM:U} == "yes"
CC+= -fno-integrated-as
.endif

regen:
	for i in $$(find ${OPENSSLSRC} -name \*${MACHINE_ARCH}\*.pl); do \
		j=$$(basename $$i .pl).S; \
		(echo "#include <machine/asm.h>"; CC=${CC:Q} perl $$i) | \
		sed -e 's/jal     ra,AES_set_encrypt_key/call    AES_set_encrypt_key/' > $$j; \
	done
