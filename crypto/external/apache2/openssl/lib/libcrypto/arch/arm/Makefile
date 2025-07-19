#	$NetBSD: Makefile,v 1.2 2025/07/19 18:36:52 christos Exp $

.include "bsd.own.mk"

CRYPTODIST=${NETBSDSRCDIR}/crypto
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"

regen:
	for i in $$(find ${OPENSSLSRC} -name \*arm\*.pl); do \
		case $$i in \
		(*aes-gcm-armv8-unroll8_64.pl);; \
		(*/charmap.pl|*/arm-xlate.pl);; \
		(*) printf '#include "arm_asm.h"\n' > $$(basename $$i .pl).S; \
		perl -I${OPENSSLSRC}/crypto/perlasm \
		-I${OPENSSLSRC}/crypto/bn/asm $$i linux /dev/stdout \
		| sed -E 's/bx[[:space:]]+lr/RET/' \
		>> $$(basename $$i .pl).S;; \
		esac; \
	done
