#	$NetBSD: Makefile,v 1.74 2022/05/29 08:10:00 nia Exp $

.include <bsd.own.mk>

SUBDIR=	acpica am-utils bc byacc cron dhcpcd elftoolchain/common \
	ekermit elftosb fetch file flex jemalloc less \
	libarchive libevent libfido2 liblzf libpcap mdocml \
	openresolv tmux top tre wpa

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
.if (${MKNTP} != "no")
SUBDIR+= ntp
.endif
.if ${MKPAM} != "no"
SUBDIR+= pam-u2f
.endif
.if (${MKPCC} != "no")
SUBDIR+= pcc
.endif
.if (${MKPPP} != "no")
SUBDIR+= ppp
.endif
.if (${MKDTRACE} != "no")
SUBDIR+= libproc
SUBDIR+= librtld_db
.endif
.if ${MKTCPDUMP} != "no"
SUBDIR+= tcpdump
.endif
.if ${MKUNBOUND} != "no"
SUBDIR+= unbound
.endif

SUBDIR+= blocklist pdisk nvi libnv

.include <bsd.subdir.mk>
