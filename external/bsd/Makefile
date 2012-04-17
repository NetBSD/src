#	$NetBSD: Makefile,v 1.30.2.2 2012/04/17 00:02:59 yamt Exp $

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
# IP Filter
.if (${MKIPFILTER} != "no")
SUBDIR+=ipf
.endif

.include <bsd.subdir.mk>
