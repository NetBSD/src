#	$NetBSD: Makefile,v 1.35 1999/08/06 22:49:06 aidan Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

SUBDIR=	atrun comsat fingerd ftpd getNAME getty identd ld.aout_so \
	ld.elf_so lfs_cleanerd mail.local makekey makewhatis rexecd \
	rlogind rmail rshd rpc.rquotad rpc.rstatd rpc.rusersd rpc.rwalld \
	rpc.sprayd talkd telnetd tftpd uucpd

.if defined(CRYPTOPATH)
.sinclude "${CRYPTOPATH}/libexec/Makefile.frag"
.endif

.include <bsd.subdir.mk>
