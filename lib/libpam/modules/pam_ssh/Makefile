# $NetBSD: Makefile,v 1.3 2004/12/29 04:16:17 christos Exp $
# PAM module for SSH
# $FreeBSD: src/lib/libpam/modules/pam_ssh/Makefile,v 1.18 2004/08/06 07:27:04 cperciva Exp $

.include <bsd.own.mk>

SSHSRC=	${NETBSDSRCDIR}/crypto/dist/ssh

LIB=	pam_ssh
MAN=	pam_ssh.8
SRCS=	pam_ssh.c

CPPFLAGS+= -I${SSHSRC}

# This does not work yet because we don't have libssh.
DPADD=	${LIBCRYPTO} ${LIBCRYPT}
LDADD=	-Wl,-L${LIBCRYPTODIR}:${LIBCRYPTDIR} -lcrypto -lcrypt

.include "${.CURDIR}/../mod.mk"
