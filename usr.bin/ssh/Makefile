#	$NetBSD: Makefile,v 1.16 2002/04/29 08:23:36 lukem Exp $

.include <bsd.own.mk>

SSHDIST?= ${.CURDIR}/../../crypto/dist/ssh

SUBDIR=	libssh .WAIT ssh sshd ssh-add ssh-keygen ssh-agent scp sftp-server \
	ssh-keyscan sftp

distribution:
	${INSTALL_FILE} -o root -g wheel -m 644 \
		${SSHDIST}/ssh_config ${DESTDIR}/etc/ssh/ssh_config
	${INSTALL_FILE} -o root -g wheel -m 644 \
		${SSHDIST}/sshd_config ${DESTDIR}/etc/ssh/sshd_config
	${INSTALL_FILE} -o root -g wheel -m 444 \
		${SSHDIST}/moduli ${DESTDIR}/etc/moduli

.include <bsd.subdir.mk>
