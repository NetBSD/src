#	from: @(#)Makefile	5.7 (Berkeley) 4/1/91
#	$Id: Makefile,v 1.20 1996/12/18 05:39:13 thorpej Exp $

.include <bsd.own.mk>			# for EXPORTABLE_SYSTEM
#					# and ELF_TOOLCHAIN

SUBDIR=	atrun comsat fingerd ftpd getNAME getty identd lfs_cleanerd \
	mail.local makekey makewhatis rexecd rlogind rshd rpc.rquotad \
	rpc.rstatd rpc.rusersd rpc.rwalld rpc.sprayd talkd \
	tftpd uucpd

# XXX Keep telnetd(8) and libtelnet from getting out of sync during the build
# XXX process.
.if !exists(${.CURDIR}/../domestic/libexec/telnetd) || \
    defined(EXPORTABLE_SYSTEM)
SUBDIR+=telned
.endif

.if ((${MACHINE_ARCH} == "alpha") && defined(ELF_TOOLCHAIN))
SUBDIR+= ld.elf_so
.endif

.include <bsd.subdir.mk>
