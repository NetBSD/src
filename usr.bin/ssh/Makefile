#	$NetBSD: Makefile,v 1.8 2000/09/28 22:20:58 thorpej Exp $

.include <bsd.own.mk>

SSHDIST?= ${.CURDIR}/../../crypto/dist/ssh

SUBDIR=	libssh ssh sshd ssh-add ssh-keygen ssh-agent scp

all-ssh all-sshd all-ssh-add all-ssh-keygen all-ssh-agent all-scp: all-libssh
dependall-ssh dependall-sshd dependall-ssh-add dependall-ssh-keygen \
	dependall-ssh-agent dependall-scp: dependall-libssh

distribution:
	${INSTALL} ${INSTPRIV} ${COPY} -o root -g wheel -m 644 \
	    ${SSHDIST}/ssh.conf ${DESTDIR}/etc/ssh.conf
	${INSTALL} ${INSTPRIV} ${COPY} -o root -g wheel -m 644 \
	    ${SSHDIST}/sshd.conf ${DESTDIR}/etc/sshd.conf

.include <bsd.subdir.mk>
