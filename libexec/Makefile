#	$NetBSD: Makefile,v 1.40 2000/08/06 20:42:22 thorpej Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

.include <bsd.own.mk>

SUBDIR=	atrun comsat fingerd ftpd getNAME getty identd ld.aout_so \
	ld.elf_so lfs_cleanerd mail.local makekey makewhatis rexecd \
	rlogind rmail rshd rpc.rquotad rpc.rstatd rpc.rusersd rpc.rwalld \
	rpc.sprayd talkd telnetd tftpd uucpd

.if (${MKCRYPTO} != "no")
# Heimdal/KTH Kerberos
SUBDIR+= hpropd kadmind kfd kpasswdd
.endif

.include <bsd.subdir.mk>
