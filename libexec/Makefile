#	$NetBSD: Makefile,v 1.45 2004/06/22 15:53:18 itojun Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

.include <bsd.own.mk>

SUBDIR=	atrun comsat fingerd ftpd getNAME getty identd ld.aout_so \
	ld.elf_so lfs_cleanerd mail.local makekey makewhatis rexecd \
	rlogind rmail rshd rpc.rquotad rpc.rstatd rpc.rusersd rpc.rwalld \
	rpc.sprayd talkd telnetd tftpd utmp_update uucpd

.if (${MKKERBEROS} != "no")
# Heimdal/KTH Kerberos
SUBDIR+= hpropd kadmind kpasswdd
.endif

SUBDIR+=login_chpass login_krb5 login_krb5-or-pwd login_lchpass \
	login_passwd login_radius login_reject login_skey login_token

# PF
SUBDIR+=spamd spamd-setup

.include <bsd.subdir.mk>
