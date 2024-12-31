# $NetBSD: Makefile,v 1.13 2024/12/31 20:27:49 christos Exp $

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
