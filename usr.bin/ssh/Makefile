#	$NetBSD: Makefile,v 1.8.2.3 2001/05/27 09:15:33 he Exp $

.include <bsd.own.mk>

SSHDIST?= ${.CURDIR}/../../crypto/dist/ssh

SUBDIR=	libssh ssh sshd ssh-add ssh-keygen ssh-agent scp sftp-server \
	ssh-keyscan sftp

distribution:
	${INSTALL} ${INSTPRIV} ${COPY} -o root -g wheel -m 644 \
	    ${SSHDIST}/ssh_config ${DESTDIR}/etc/ssh.conf
	${INSTALL} ${INSTPRIV} ${COPY} -o root -g wheel -m 644 \
	    ${SSHDIST}/sshd_config ${DESTDIR}/etc/sshd.conf
	${INSTALL} ${INSTPRIV} ${COPY} -o root -g wheel -m 444 \
	    ${SSHDIST}/primes ${DESTDIR}/etc/primes

.include <bsd.subdir.mk>
