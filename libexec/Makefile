#	$NetBSD: Makefile,v 1.62 2022/05/27 07:28:20 nia Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

.include <bsd.own.mk>

SUBDIR= atrun \
	comsat fingerd ftpd getty httpd identd ld.aout_so \
	ld.elf_so mail.local makekey rexecd \
	rlogind rshd rpc.rquotad rpc.rstatd rpc.rusersd rpc.rwalld \
	rpc.sprayd talkd telnetd tftpd utmp_update

.if ${MKLFS} != "no"
SUBDIR+= lfs_cleanerd
.endif

.if ${MKMAKEMANDB} == "no"
SUBDIR+= getNAME makewhatis
.endif

.include <bsd.subdir.mk>
