#	$NetBSD: Makefile,v 1.3 2025/07/20 00:32:10 christos Exp $

.include "bsd.own.mk"

CRYPTODIST=${NETBSDSRCDIR}/crypto
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"

.if make(regen) && ${HAVE_LLVM:U} == "yes"
CC+= -fno-integrated-as
.endif

regen:
	for i in $$(find ${OPENSSLSRC} -name \*${MACHINE_ARCH}\*.pl); do \
		j=$$(basename $$i .pl).S; \
		case $$j in \
		(chacha-riscv64-v-zbb.S) \
		    (echo "#include <machine/asm.h>"; CC=${CC:Q} perl $$i zvkb)  > chacha-riscv64-v-zvkb.S;; \
		esac; \
		(echo "#include <machine/asm.h>"; CC=${CC:Q} perl $$i) | \
		sed -e 's/jal     ra,AES_set_encrypt_key/call    AES_set_encrypt_key/' > $$j; \
	done
