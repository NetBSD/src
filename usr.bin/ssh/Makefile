#	$NetBSD: Makefile,v 1.20 2005/01/03 06:04:08 lukem Exp $

.include <bsd.own.mk>

SSHDIST?= ${NETBSDSRCDIR}/crypto/dist/ssh

SUBDIR=	ssh sshd ssh-add ssh-keygen ssh-agent scp sftp-server \
	ssh-keysign ssh-keyscan sftp
#SUBDIR+=scard

.PATH: ${SSHDIST}
CONFIGFILES=	ssh_config sshd_config moduli
FILESDIR=	/etc/ssh
FILESDIR_moduli=/etc

.include <bsd.files.mk>
.include <bsd.subdir.mk>
