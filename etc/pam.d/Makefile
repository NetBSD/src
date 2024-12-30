# $NetBSD: Makefile,v 1.12 2024/12/30 21:59:08 jmmv Exp $

.include <bsd.own.mk>

CONFIGFILES=	README cron display_manager ftpd gdm imap kde login other \
		passwd pop3 ppp racoon rexecd rsh sshd su system telnetd xdm \
		xserver
FILESDIR=	/etc/pam.d
FILESMODE=	644
FILESMODE_README=444

.for f in display_manager ftpd sshd su system
FILESBUILD_$f=	yes

$f: $f.in
	${_MKTARGET_CREATE}
.if ${MKSKEY} != "no"
	cp $> $@
.else
	${TOOL_SED} -e '/pam_skey/d' < $> > $@
.endif
.endfor

.include <bsd.prog.mk>
