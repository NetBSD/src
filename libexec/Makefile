#	$NetBSD: Makefile,v 1.50 2006/11/12 01:27:29 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

.include <bsd.own.mk>

SUBDIR=	atrun comsat fingerd ftpd getNAME getty identd ld.aout_so \
	ld.elf_so lfs_cleanerd mail.local makekey makewhatis rexecd \
	rlogind rshd rpc.rquotad rpc.rstatd rpc.rusersd rpc.rwalld \
	rpc.sprayd talkd telnetd tftpd utmp_update uucpd

.if (${MKKERBEROS} != "no")
# Heimdal/KTH Kerberos
SUBDIR+= hpropd kadmind kpasswdd
.endif

.include <bsd.subdir.mk>
