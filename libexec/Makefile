#	$NetBSD: Makefile,v 1.26 1997/06/23 05:15:44 mrg Exp $
#	from: @(#)Makefile	5.7 (Berkeley) 4/1/91

.include <bsd.own.mk>			# for EXPORTABLE_SYSTEM
#					# and ECOFF_TOOLCHAIN

SUBDIR=	atrun comsat fingerd ftpd getNAME getty identd ld.elf_so lfs_cleanerd \
	mail.local makekey makewhatis rexecd rlogind rmail rshd rpc.rquotad \
	rpc.rstatd rpc.rusersd rpc.rwalld rpc.sprayd talkd \
	tftpd uucpd

# XXX Keep telnetd(8) and libtelnet from getting out of sync during the build
# XXX process.
.if !exists(${.CURDIR}/../domestic/libexec/telnetd) || \
    defined(EXPORTABLE_SYSTEM) || make(clean) || make(cleandir)
SUBDIR+=telnetd
.endif

.include <bsd.subdir.mk>
