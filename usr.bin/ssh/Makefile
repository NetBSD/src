#	$NetBSD: Makefile,v 1.9 2001/01/09 03:13:41 lukem Exp $

.include <bsd.own.mk>

SSHDIST?= ${.CURDIR}/../../crypto/dist/ssh

SUBDIR=	libssh .WAIT ssh sshd ssh-add ssh-keygen ssh-agent scp

distribution:
	${INSTALL} ${INSTPRIV} ${COPY} -o root -g wheel -m 644 \
	    ${SSHDIST}/ssh.conf ${DESTDIR}/etc/ssh.conf
	${INSTALL} ${INSTPRIV} ${COPY} -o root -g wheel -m 644 \
	    ${SSHDIST}/sshd.conf ${DESTDIR}/etc/sshd.conf

.include <bsd.subdir.mk>
