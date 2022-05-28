#	$NetBSD: Makefile,v 1.63 2022/05/28 14:31:11 nia Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

.include <bsd.own.mk>

SUBDIR= atrun \
	comsat ftpd getty httpd identd ld.aout_so \
	ld.elf_so mail.local makekey rexecd \
	rlogind rshd rpc.rquotad rpc.rstatd rpc.rusersd rpc.rwalld \
	rpc.sprayd telnetd tftpd utmp_update

.if ${MKFINGER} != "no"
SUBDIR+= fingerd
.endif

.if ${MKLFS} != "no"
SUBDIR+= lfs_cleanerd
.endif

.if ${MKMAKEMANDB} == "no"
SUBDIR+= getNAME makewhatis
.endif

.if ${MKTALK} != "no"
SUBDIR+= talkd
.endif

.include <bsd.subdir.mk>
