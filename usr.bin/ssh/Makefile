#	$NetBSD: Makefile,v 1.4 2000/07/28 16:43:49 thorpej Exp $

.include <bsd.own.mk>

.include "${.CURDIR}/Makefile.inc"

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
