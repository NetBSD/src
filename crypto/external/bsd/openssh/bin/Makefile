#	$NetBSD: Makefile,v 1.2 2009/07/21 00:47:23 mrg Exp $

.include <bsd.own.mk>

SSHDIST?= ${NETBSDSRCDIR}/crypto/external/bsd/openssh/dist

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
