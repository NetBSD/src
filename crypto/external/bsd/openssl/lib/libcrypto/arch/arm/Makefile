#	$NetBSD: Makefile,v 1.2.2.1 2018/09/30 01:44:52 pgoyette Exp $

.include "bsd.own.mk"

CRYPTODIST=${NETBSDSRCDIR}/crypto
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"

regen:
	for i in $$(find ${OPENSSLSRC} -name \*arm\*.pl); do \
		case $$i in \
		(*/charmap.pl|*/arm-xlate.pl);; \
		(*) printf '#include "arm_asm.h"\n' > $$(basename $$i .pl).S; \
		perl -I${OPENSSLSRC}/crypto/perlasm \
		-I${OPENSSLSRC}/crypto/bn/asm $$i linux /dev/stdout \
		| sed -E 's/bx[[:space:]]+lr/RET/' \
		>> $$(basename $$i .pl).S;; \
		esac; \
	done
