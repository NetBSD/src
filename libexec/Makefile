#	$NetBSD: Makefile,v 1.47 2004/06/22 22:19:36 itojun Exp $
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

# PF
SUBDIR+=spamd spamd-setup ftp-proxy

.include <bsd.subdir.mk>
