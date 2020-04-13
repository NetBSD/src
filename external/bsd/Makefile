#	$NetBSD: Makefile,v 1.59.2.3 2020/04/13 07:45:48 martin Exp $

.include <bsd.own.mk>

SUBDIR=	acpica am-utils bc byacc cron dhcpcd ekermit elftosb \
	fetch file flex jemalloc less \
	libarchive libevent libfido2 liblzf libpcap mdocml \
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
.if (${MKNSD} != "no")
SUBDIR+= nsd
.endif
.if ${MKPAM} != "no"
SUBDIR+= pam-u2f
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

SUBDIR+= blacklist pdisk nvi ppp libnv

.include <bsd.subdir.mk>
