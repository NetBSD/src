# $NetBSD: Makefile,v 1.7 2008/10/27 07:57:41 mrg Exp $
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

LIBDPLIBS+=	ssh	${.CURDIR}/../../../libssh \
		crypt	${.CURDIR}/../../../libcrypt \
		crypto	${.CURDIR}/../../../libcrypto

.include "${.CURDIR}/../mod.mk"
