#	$NetBSD: Makefile,v 1.56 2008/02/04 20:31:32 jmmv Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

.include <bsd.own.mk>

SUBDIR=	atf-cleanup atf-exec atf-format atf-killpg atrun \
	comsat fingerd ftpd getNAME getty httpd identd ld.aout_so \
	ld.elf_so lfs_cleanerd mail.local makekey makewhatis rexecd \
	rlogind rshd rpc.rquotad rpc.rstatd rpc.rusersd rpc.rwalld \
	rpc.sprayd talkd telnetd tftpd utmp_update

.if (${MKKERBEROS} != "no")
# Heimdal/KTH Kerberos
SUBDIR+= hpropd kadmind kpasswdd
.endif

.include <bsd.subdir.mk>
