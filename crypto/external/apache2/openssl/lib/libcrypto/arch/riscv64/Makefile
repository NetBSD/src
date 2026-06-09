#	$NetBSD: Makefile,v 1.4 2026/06/09 19:20:33 christos Exp $

.include "bsd.own.mk"

CRYPTODIST=${NETBSDSRCDIR}/crypto
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"

.if make(regen) && ${HAVE_LLVM:U} == "yes"
CC+= -fno-integrated-as
.endif

regen:
	for i in $$(find ${OPENSSLSRC} -name \*riscv64\*.pl); do \
		j=$$(basename $$i .pl).S; \
		case $$j in \
		(chacha-riscv64-v-zbb.S) \
		    (echo "#include <machine/asm.h>"; CC=${CC:Q} perl $$i zvkb)  > chacha-riscv64-v-zvkb.S;; \
		esac; \
		(echo "#include <machine/asm.h>"; CC=${CC:Q} perl $$i) | \
		sed -e 's/jal     ra,AES_set_encrypt_key/call    AES_set_encrypt_key/' > $$j; \
	done
