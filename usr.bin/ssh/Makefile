#	$NetBSD: Makefile,v 1.13 2001/06/23 19:37:43 itojun Exp $

.include <bsd.own.mk>

SSHDIST?= ${.CURDIR}/../../crypto/dist/ssh

SUBDIR=	libssh .WAIT ssh sshd ssh-add ssh-keygen ssh-agent scp sftp-server \
	ssh-keyscan sftp

distribution:
	${INSTALL} ${INSTPRIV} ${COPY} -o root -g wheel -m 644 \
	    ${SSHDIST}/ssh_config ${DESTDIR}/etc/ssh.conf
	${INSTALL} ${INSTPRIV} ${COPY} -o root -g wheel -m 644 \
	    ${SSHDIST}/sshd_config ${DESTDIR}/etc/sshd.conf
	${INSTALL} ${INSTPRIV} ${COPY} -o root -g wheel -m 444 \
	    ${SSHDIST}/moduli ${DESTDIR}/etc/moduli

.include <bsd.subdir.mk>
