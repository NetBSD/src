# $NetBSD: Makefile,v 1.3.2.2 2020/04/13 07:56:29 martin Exp $

NOMAN=
.include <bsd.own.mk>
.PATH: ${NETBSDSRCDIR}/external/bsd/pam-u2f/dist

CPPFLAGS+= -DHAVE_SECURITY_PAM_APPL_H #-DHAVE_SECURITY_PAM_MODULES_H
#CPPFLAGS+= -DPAM_DEBUG -DDEBUG_PAM

LIB=	pam_u2f
SRCS=	pam-u2f.c drop_privs.c util.c b64.c explicit_bzero.c
#MAN=	pam_u2f.8

COPTS.util.c += -Wno-error=stack-protector

LIBDPLIBS+= \
	fido2	${NETBSDSRCDIR}/external/bsd/libfido2/lib \
	cbor	${NETBSDSRCDIR}/external/mit/libcbor/lib \
	crypto	${NETBSDSRCDIR}/crypto/external/bsd/${EXTERNAL_OPENSSL_SUBDIR}/lib/libcrypto \
	m	${NETBSDSRCDIR}/lib/libm

.include "${NETBSDSRCDIR}/lib/libpam/modules/mod.mk"
