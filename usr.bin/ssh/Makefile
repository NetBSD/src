#	$NetBSD: Makefile,v 1.6 2000/09/23 06:18:08 simonb Exp $

.include <bsd.own.mk>

#SSHDIST=${.CURDIR}/../../crypto/dist/ssh

.if defined(SSHDIST)
SUBDIR=	libssh ssh sshd ssh-add ssh-keygen ssh-agent scp

all-ssh all-sshd all-ssh-add all-ssh-keygen all-ssh-agent all-scp: all-libssh
dependall-ssh dependall-sshd dependall-ssh-add dependall-ssh-keygen \
	dependall-ssh-agent dependall-scp: dependall-libssh

distribution:
	${INSTALL} ${INSTPRIV} -c -o root -g wheel -m 644 \
	    ${SSHDIST}/ssh.conf ${DESTDIR}/etc/ssh.conf
	${INSTALL} ${INSTPRIV} -c -o root -g wheel -m 644 \
	    ${SSHDIST}/sshd.conf ${DESTDIR}/etc/sshd.conf
.else
distribution:
	@true
.endif

.include <bsd.subdir.mk>
