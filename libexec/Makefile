#	$NetBSD: Makefile,v 1.32 1999/02/12 17:17:57 tv Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

.include <bsd.own.mk>			# for EXPORTABLE_SYSTEM

SUBDIR=	atrun comsat fingerd ftpd getNAME getty identd \
	lfs_cleanerd mail.local makekey makewhatis rexecd rlogind rmail \
	rshd rpc.rquotad rpc.rstatd rpc.rusersd rpc.rwalld rpc.sprayd \
	talkd telnetd tftpd uucpd

.if (${MACHINE_ARCH} != "arm32")
SUBDIR+=ld.aout_so
.endif
.if (${MACHINE_ARCH} != "alpha")
SUBDIR+=ld.elf_so
.endif

.include <bsd.subdir.mk>
