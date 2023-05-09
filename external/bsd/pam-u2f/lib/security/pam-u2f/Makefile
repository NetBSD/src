# $NetBSD: Makefile,v 1.6 2023/05/09 15:02:59 christos Exp $

.include <bsd.own.mk>
.PATH: ${NETBSDSRCDIR}/external/bsd/pam-u2f/dist

CPPFLAGS+= -DHAVE_SECURITY_PAM_APPL_H #-DHAVE_SECURITY_PAM_MODULES_H
#CPPFLAGS+= -DPAM_DEBUG -DDEBUG_PAM

LIB=	pam_u2f
SRCS=	pam-u2f.c drop_privs.c util.c b64.c explicit_bzero.c
MAN=	pam_u2f.8

COPTS.util.c += -Wno-error=stack-protector -Wno-error=deprecated-declarations

LIBDPLIBS+= \
	fido2	${NETBSDSRCDIR}/external/bsd/libfido2/lib \
	cbor	${NETBSDSRCDIR}/external/mit/libcbor/lib \
	crypto	${NETBSDSRCDIR}/crypto/external/bsd/${EXTERNAL_OPENSSL_SUBDIR}/lib/libcrypto \
	m	${NETBSDSRCDIR}/lib/libm

pam_u2f.8:
	asciidoc -b docbook45 -d manpage -o pam_u2f.8.xml ../../../dist/man/pam_u2f.8.txt
	xsltproc --nonet --xinclude -o pam_u2f.8 'http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl' pam_u2f.8.xml
	rm -f pam_u2f.8.xml

.include "${NETBSDSRCDIR}/lib/libpam/modules/mod.mk"
