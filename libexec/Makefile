#	$NetBSD: Makefile,v 1.25 1997/04/09 06:23:41 mikel Exp $
#	from: @(#)Makefile	5.7 (Berkeley) 4/1/91

.include <bsd.own.mk>			# for EXPORTABLE_SYSTEM
#					# and ECOFF_TOOLCHAIN

SUBDIR=	atrun comsat fingerd ftpd getNAME getty identd lfs_cleanerd \
	mail.local makekey makewhatis rexecd rlogind rmail rshd rpc.rquotad \
	rpc.rstatd rpc.rusersd rpc.rwalld rpc.sprayd talkd \
	tftpd uucpd

.if	make(clean) || make(cleandir)
SUBDIR+=ld.elf_so telnetd
.else

# XXX Keep telnetd(8) and libtelnet from getting out of sync during the build
# XXX process.
.if !exists(${.CURDIR}/../domestic/libexec/telnetd) || \
    defined(EXPORTABLE_SYSTEM)
SUBDIR+=telnetd
.endif

.if (${MACHINE_ARCH} == "alpha" && !defined(ECOFF_TOOLCHAIN))
SUBDIR+= ld.elf_so
.endif

.endif	# make(clean) || make(cleandir)

.include <bsd.subdir.mk>
