#	$NetBSD: Makefile,v 1.5 2000/08/02 17:04:23 thorpej Exp $

.include <bsd.own.mk>

#SSHDIST=${.CURDIR}/../../crypto/dist/ssh

.if defined(SSHDIST)
SUBDIR=	libssh ssh sshd ssh-add ssh-keygen ssh-agent scp

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
