# $FreeBSD: src/etc/pam.d/Makefile,v 1.11 2004/10/24 15:32:24 ru Exp $

NOOBJ=
FILES=	README ftpd gdm imap kde login other passwd pop3 \
	rexecd rsh sshd su system telnetd xdm
FILESDIR=	/etc/pam.d
FILESMODE=	644
FILESMODE_README=	444
LINKS=	${FILESDIR}/ftpd ${FILESDIR}/ftp

.include <bsd.prog.mk>
