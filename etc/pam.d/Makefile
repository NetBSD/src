# $NetBSD: Makefile,v 1.11.18.1 2025/08/02 05:20:44 perseant Exp $

.include <bsd.own.mk>

CONFIGFILES=	README cron display_manager ftpd gdm imap kde login other \
		passwd pop3 ppp racoon rexecd rsh sshd su system telnetd xdm \
		xserver
FILESDIR=	/etc/pam.d
FILESMODE=	644
FILESMODE_README=444

.for f in display_manager ftpd sshd su system
FILESBUILD_$f=	yes
.endfor

.SUFFIXES: .in

.in:
	${_MKTARGET_CREATE}
.if ${MKSKEY} != "no"
	cp ${.IMPSRC} ${.TARGET}
.else
	${TOOL_SED} -e '/pam_skey/d' < ${.IMPSRC} > ${.TARGET}
.endif

.include <bsd.prog.mk>
