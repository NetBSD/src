#	$NetBSD: Makefile,v 1.28 1998/03/28 00:53:09 cgd Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

.include <bsd.own.mk>			# for EXPORTABLE_SYSTEM

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
