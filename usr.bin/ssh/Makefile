#	$NetBSD: Makefile,v 1.21 2006/02/15 15:51:37 christos Exp $

.include <bsd.own.mk>

SSHDIST?= ${NETBSDSRCDIR}/crypto/dist/ssh

SUBDIR=	ssh sshd ssh-add ssh-keygen ssh-agent scp sftp-server \
	ssh-keysign ssh-keyscan sftp
#SUBDIR+=scard

.PATH: ${SSHDIST}
.MADE: moduli
CONFIGFILES=	ssh_config sshd_config moduli
FILESDIR=	/etc/ssh
FILESDIR_moduli=/etc

.include <bsd.files.mk>
.include <bsd.subdir.mk>
