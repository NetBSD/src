#	$NetBSD: Makefile,v 1.38 2000/06/23 06:01:14 thorpej Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

.include <bsd.own.mk>

SUBDIR=	atrun comsat fingerd ftpd getNAME getty identd ld.aout_so \
	ld.elf_so lfs_cleanerd mail.local makekey makewhatis rexecd \
	rlogind rmail rshd rpc.rquotad rpc.rstatd rpc.rusersd rpc.rwalld \
	rpc.sprayd talkd telnetd tftpd uucpd

.if (${MKCRYPTO} != "no")
# Heimdal/KTH Kerberos
SUBDIR+= hprop hpropd kadmind kdc kfd kpasswdd
.endif

.include <bsd.subdir.mk>
