#	$NetBSD: Makefile,v 1.23 1997/02/23 20:42:27 cgd Exp $
#	from: @(#)Makefile	5.7 (Berkeley) 4/1/91

.include <bsd.own.mk>			# for EXPORTABLE_SYSTEM
#					# and ECOFF_TOOLCHAIN

SUBDIR=	atrun comsat fingerd ftpd getNAME getty identd lfs_cleanerd \
	mail.local makekey makewhatis rexecd rlogind rshd rpc.rquotad \
	rpc.rstatd rpc.rusersd rpc.rwalld rpc.sprayd talkd \
	tftpd uucpd

# XXX Keep telnetd(8) and libtelnet from getting out of sync during the build
# XXX process.
.if !exists(${.CURDIR}/../domestic/libexec/telnetd) || \
    defined(EXPORTABLE_SYSTEM)
SUBDIR+=telnetd
.endif

.if (${MACHINE_ARCH} == "alpha" && !defined(ECOFF_TOOLCHAIN))
SUBDIR+= ld.elf_so
.endif

.include <bsd.subdir.mk>
