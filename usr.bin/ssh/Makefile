#	$NetBSD: Makefile,v 1.19 2004/05/16 09:53:10 lukem Exp $

.include <bsd.own.mk>

SSHDIST?= ${NETBSDSRCDIR}/crypto/dist/ssh

SUBDIR=	libssh .WAIT ssh sshd ssh-add ssh-keygen ssh-agent scp sftp-server \
	ssh-keysign ssh-keyscan sftp
#SUBDIR+=scard

.PATH: ${SSHDIST}
CONFIGFILES=	ssh_config sshd_config moduli
FILESDIR=	/etc/ssh
FILESDIR_moduli=/etc

.include <bsd.files.mk>
.include <bsd.subdir.mk>
