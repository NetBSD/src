#	$NetBSD: Makefile,v 1.31 1998/12/30 15:07:08 tv Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

.include <bsd.own.mk>			# for EXPORTABLE_SYSTEM

SUBDIR=	atrun comsat fingerd ftpd getNAME getty identd ld.elf_so \
	lfs_cleanerd mail.local makekey makewhatis rexecd rlogind rmail \
	rshd rpc.rquotad rpc.rstatd rpc.rusersd rpc.rwalld rpc.sprayd \
	talkd telnetd tftpd uucpd

.if (${MACHINE_ARCH} != "arm32")
SUBDIR+=ld.aout_so
.endif

.include <bsd.subdir.mk>
