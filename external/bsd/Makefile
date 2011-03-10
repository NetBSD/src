#	$NetBSD: Makefile,v 1.29 2011/03/10 13:16:57 jmmv Exp $

.include <bsd.own.mk>

SUBDIR=	acpica am-utils bind cron dhcpcd fetch file flex libarchive \
	libevent liblzf libpcap mdocml openresolv top byacc wpa ntp tcpdump \
	tmux

.if (${MKATF} != "no")
SUBDIR+= atf
.endif
.if (${MKISCSI} != "no")
SUBDIR+= iscsi 
.endif
.if (${MKLDAP} != "no")
SUBDIR+= openldap
.endif
.if (${MKLLVM} != "no")
SUBDIR+= llvm
.endif
.if (${MKCRYPTO} != "no")
SUBDIR+= pkg_install ../../crypto/external/bsd
.endif
.if (${MKPCC} != "no")
SUBDIR+= pcc
.endif

.include <bsd.subdir.mk>
