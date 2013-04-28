#	$NetBSD: Makefile,v 1.43 2013/04/28 00:14:14 joerg Exp $

.include <bsd.own.mk>

SUBDIR=	acpica am-utils bind byacc cron dhcpcd elftosb fetch file flex less \
	libarchive libevent liblzf libpcap mdocml ntp openresolv tcpdump \
	tmux top tre wpa

.if (${MKATF} != "no")
SUBDIR+= atf
.endif
.if (${MKCRYPTO} != "no")
SUBDIR+= pkg_install
.endif
# IP Filter
.if (${MKIPFILTER} != "no")
SUBDIR+=ipf
.endif
.if (${MKISCSI} != "no")
SUBDIR+= iscsi 
.endif
.if (${MKKYUA} != "no")
SUBDIR+= lutok kyua-testers kyua-cli kyua-atf-compat
.endif
.if (${MKLDAP} != "no")
SUBDIR+= openldap
.endif
.if (${MKLIBCXX} != "no")
SUBDIR+= libc++
.endif
.if (${MKLLVM} != "no")
SUBDIR+= llvm
.endif
.if (${MKPCC} != "no")
SUBDIR+= pcc
.endif

SUBDIR+= pdisk dhcp

.include <bsd.subdir.mk>
