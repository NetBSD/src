#	$NetBSD: Makefile,v 1.17 2002/06/24 05:48:24 itojun Exp $

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
