#	$NetBSD: Makefile,v 1.60.4.1 2012/04/17 00:05:35 yamt Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

.include <bsd.own.mk>

SUBDIR= atrun \
	comsat fingerd ftpd getty httpd identd ld.aout_so \
	ld.elf_so lfs_cleanerd mail.local makekey rexecd \
	rlogind rshd rpc.rquotad rpc.rstatd rpc.rusersd rpc.rwalld \
	rpc.sprayd talkd telnetd tftpd utmp_update

.if ${MKMAKEMANDB} == "no"
SUBDIR+= getNAME makewhatis
.endif

.include <bsd.subdir.mk>
