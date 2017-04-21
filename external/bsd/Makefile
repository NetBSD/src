#	$NetBSD: Makefile,v 1.56.2.1 2017/04/21 16:51:17 bouyer Exp $

.include <bsd.own.mk>

SUBDIR=	acpica am-utils bc bind byacc cron dhcpcd ekermit elftosb \
	fetch file flex less \
	libarchive libevent liblzf libpcap mdocml \
	ntp openresolv tcpdump tmux top tre wpa

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
.if (${MKNSD} != "no")
SUBDIR+= nsd
.endif
.if (${MKPCC} != "no")
SUBDIR+= pcc
.endif
.if (${MKDTRACE} != "no")
SUBDIR+= libproc
SUBDIR+= librtld_db
.endif
.if (${MKCRYPTO} != "no" && ${MKUNBOUND} != "no")
SUBDIR+= unbound
.endif

SUBDIR+= blacklist pdisk dhcp nvi ppp smbfs

.include <bsd.subdir.mk>
