#	$NetBSD: Makefile,v 1.16.2.1 2002/06/26 16:54:43 tv Exp $

.include <bsd.own.mk>

SSHDIST?= ${.CURDIR}/../../crypto/dist/ssh

SUBDIR=	libssh .WAIT ssh sshd ssh-add ssh-keygen ssh-agent scp sftp-server \
	ssh-keysign ssh-keyscan sftp
#SUBDIR+=scard

distribution:
	${INSTALL_FILE} -o root -g wheel -m 644 \
		${SSHDIST}/ssh_config ${DESTDIR}/etc/ssh/ssh_config
	${INSTALL_FILE} -o root -g wheel -m 644 \
		${SSHDIST}/sshd_config ${DESTDIR}/etc/ssh/sshd_config
	${INSTALL_FILE} -o root -g wheel -m 444 \
		${SSHDIST}/moduli ${DESTDIR}/etc/moduli

.include <bsd.subdir.mk>
