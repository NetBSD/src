#	$NetBSD: Makefile,v 1.3 2000/07/26 15:16:28 simonb Exp $

.include <bsd.own.mk>

.include "${.CURDIR}/Makefile.inc"

.if defined(SSHDIST)
SUBDIR=	libssh ssh sshd ssh-add ssh-keygen ssh-agent scp

distribution:
	${INSTALL} ${INSTPRIV} -c -o root -g wheel -m 644 \
	    ${SSHDIST}/ssh_config ${DESTDIR}/etc/ssh_config
	${INSTALL} ${INSTPRIV} -c -o root -g wheel -m 644 \
	    ${SSHDIST}/sshd_config ${DESTDIR}/etc/sshd_config
.else
distribution:
	@true
.endif

.include <bsd.subdir.mk>
