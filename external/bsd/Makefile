#	$NetBSD: Makefile,v 1.31 2011/11/05 22:40:26 christos Exp $

.include <bsd.own.mk>

SUBDIR=	acpica am-utils bind cron dhcpcd fetch file flex less libarchive \
	libevent liblzf libpcap mdocml openresolv top byacc wpa ntp tcpdump \
	tmux tre

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
