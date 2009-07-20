# $NetBSD: Makefile,v 1.9 2009/07/20 17:29:08 christos Exp $
# PAM module for SSH
# $FreeBSD: src/lib/libpam/modules/pam_ssh/Makefile,v 1.18 2004/08/06 07:27:04 cperciva Exp $

NOLINT=		# don't build a lint library
NOPROFILE=	# don't build a profile library
NOPICINSTALL=	# don't install _pic.a library

.include <bsd.own.mk>

SSHSRC=	${NETBSDSRCDIR}/crypto/dist/ssh

LIB=	pam_ssh
MAN=	pam_ssh.8
SRCS=	pam_ssh.c

CPPFLAGS+= -I${SSHSRC}

LIBDPLIBS+=	ssh	${NETBSDSRCDIR}/crypto/external/bsd/openssh/lib \
		crypt	${.CURDIR}/../../../libcrypt \
		crypto	${NETBSDSRCDIR}/crypto/external/bsd/openssl/lib/libcrypto

.include "${.CURDIR}/../mod.mk"
