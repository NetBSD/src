#	$NetBSD: Makefile,v 1.6 2018/02/18 23:38:47 christos Exp $

.include "bsd.own.mk"

CRYPTODIST=${NETBSDSRCDIR}/crypto
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"

regen:
	for i in $$(find ${OPENSSLSRC} -name \*sparcv9\*.pl) \
		 ${OPENSSLSRC}/crypto/bn/asm/sparct4-mont.pl \
		 ${OPENSSLSRC}/crypto/bn/asm/vis3-mont.pl; do \
		j=$$(basename $$i .pl).S; \
		case $$j in \
		sparc*_modes.pl|sha1-*) perl $$i $$j;; \
		*) perl $$i > $$j;; \
		esac; \
	done
	#cp ${OPENSSLSRC}/crypto/bn/asm/sparcv8plus.S bn-sparcv9.S
	m4 ${OPENSSLSRC}/crypto/des/asm/des_enc.m4 | \
		sed 's,OPENSSL_SYSNAME_ULTRASPARC,__sparc_v9__,g' | \
		sed 's,\.PIC\.DES_SPtrans,_PIC_DES_SPtrans,g' > des_enc-sparc.S
foo:
