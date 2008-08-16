#	$NetBSD: Makefile,v 1.58 2008/08/16 23:51:15 gmcgarry Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

.include <bsd.own.mk>

SUBDIR= atrun \
	comsat fingerd ftpd getNAME getty httpd identd ld.aout_so \
	ld.elf_so lfs_cleanerd mail.local makekey makewhatis rexecd \
	rlogind rshd rpc.rquotad rpc.rstatd rpc.rusersd rpc.rwalld \
	rpc.sprayd talkd telnetd tftpd utmp_update

.if (${MKATF} != "no")
SUBDIR+= atf-cleanup atf-exec atf-format atf-killpg
.endif

.if (${MKKERBEROS} != "no")
# Heimdal/KTH Kerberos
SUBDIR+= hpropd kadmind kpasswdd ipropd-master ipropd-slave
.endif

.include <bsd.subdir.mk>
