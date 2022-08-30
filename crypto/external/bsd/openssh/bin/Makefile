#	$NetBSD: Makefile,v 1.5 2022/08/30 13:40:37 riastradh Exp $

.include <bsd.own.mk>

SSHDIST?= ${NETBSDSRCDIR}/crypto/external/bsd/openssh/dist

SUBDIR=	ssh sshd ssh-add ssh-copy-id ssh-keygen ssh-agent scp \
	sftp-server ssh-keysign ssh-keyscan sftp ssh-pkcs11-helper

.PATH: ${SSHDIST}
.MADE: moduli
CONFIGFILES=	ssh_config sshd_config moduli
FILESDIR=	/etc/ssh
FILESMODE=	644
FILESDIR_moduli=/etc
FILESMODE_moduli=444

.include <bsd.files.mk>
.include <bsd.subdir.mk>
