#	from: @(#)Makefile	5.7 (Berkeley) 4/1/91
#	$Id: Makefile,v 1.19 1996/12/16 20:01:34 cgd Exp $

.include <bsd.own.mk>

SUBDIR=	atrun comsat fingerd ftpd getNAME getty identd lfs_cleanerd \
	mail.local makekey makewhatis rexecd rlogind rshd rpc.rquotad \
	rpc.rstatd rpc.rusersd rpc.rwalld rpc.sprayd talkd telnetd \
	tftpd uucpd

.if ((${MACHINE_ARCH} == "alpha") && defined(ELF_TOOLCHAIN))
SUBDIR+= ld.elf_so
.endif

.include <bsd.subdir.mk>
