#	$NetBSD: Makefile,v 1.10 2001/01/14 05:22:33 itojun Exp $

.include <bsd.own.mk>

SSHDIST?= ${.CURDIR}/../../crypto/dist/ssh

SUBDIR=	libssh .WAIT ssh sshd ssh-add ssh-keygen ssh-agent scp \
	ssh-keyscan sftp-server

distribution:
	${INSTALL} ${INSTPRIV} ${COPY} -o root -g wheel -m 644 \
	    ${SSHDIST}/ssh.conf ${DESTDIR}/etc/ssh.conf
	${INSTALL} ${INSTPRIV} ${COPY} -o root -g wheel -m 644 \
	    ${SSHDIST}/sshd.conf ${DESTDIR}/etc/sshd.conf

.include <bsd.subdir.mk>
