# $NetBSD: Makefile,v 1.4 2004/12/29 15:04:06 lukem Exp $
# PAM module for SSH
# $FreeBSD: src/lib/libpam/modules/pam_ssh/Makefile,v 1.18 2004/08/06 07:27:04 cperciva Exp $

.include <bsd.own.mk>	# XXX: fubars mod.mk's NOxxx

SSHSRC=	${NETBSDSRCDIR}/crypto/dist/ssh

LIB=	pam_ssh
MAN=	pam_ssh.8
SRCS=	pam_ssh.c

CPPFLAGS+= -I${SSHSRC}

# This does not work yet because we don't have libssh.
LIBDPLIBS=	crypt	${LIB_ROOT_DIR}/libcrypt \
		crypto	${LIB_ROOT_DIR}/libcrypto

.include "${.CURDIR}/../mod.mk"
