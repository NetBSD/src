#	$NetBSD: Makefile,v 1.33 1999/02/27 03:34:05 tv Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

SUBDIR=	atrun comsat fingerd ftpd getNAME getty identd ld.aout_so \
	lfs_cleanerd mail.local makekey makewhatis rexecd rlogind rmail \
	rshd rpc.rquotad rpc.rstatd rpc.rusersd rpc.rwalld rpc.sprayd \
	talkd telnetd tftpd uucpd

.if (${MACHINE_ARCH} != "alpha")
SUBDIR+=ld.elf_so
.endif

.include <bsd.subdir.mk>
