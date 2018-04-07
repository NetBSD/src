#	$NetBSD: Makefile,v 1.59 2018/04/07 22:39:53 christos Exp $

.include <bsd.own.mk>

SUBDIR=	acpica am-utils bc bind byacc cron dhcpcd ekermit elftosb \
	fetch file flex less \
	libarchive libevent liblzf libpcap mdocml \
	ntp openresolv tcpdump tmux top tre wpa

.if (${MKATF} != "no")
SUBDIR+= atf
.endif
SUBDIR+= pkg_install
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
.if ${MKUNBOUND} != "no"
SUBDIR+= unbound
.endif

SUBDIR+= blacklist pdisk nvi ppp smbfs

.include <bsd.subdir.mk>
