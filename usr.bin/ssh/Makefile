#	$NetBSD: Makefile,v 1.11 2001/02/07 17:05:36 itojun Exp $

.include <bsd.own.mk>

SSHDIST?= ${.CURDIR}/../../crypto/dist/ssh

SUBDIR=	libssh .WAIT ssh sshd ssh-add ssh-keygen ssh-agent scp sftp-server \
	ssh-keyscan sftp

distribution:
	${INSTALL} ${INSTPRIV} ${COPY} -o root -g wheel -m 644 \
	    ${SSHDIST}/ssh_config ${DESTDIR}/etc/ssh.conf
	${INSTALL} ${INSTPRIV} ${COPY} -o root -g wheel -m 644 \
	    ${SSHDIST}/sshd_config ${DESTDIR}/etc/sshd.conf

.include <bsd.subdir.mk>
