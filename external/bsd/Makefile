#	$NetBSD: Makefile,v 1.81 2025/12/28 15:35:01 thorpej Exp $

.include <bsd.own.mk>

SUBDIR=	acpica am-utils bc byacc cron dhcpcd elftoolchain/common \
	ekermit elftosb fetch file flex ${EXTERNAL_JEMALLOC_SUBDIR} less \
	libarchive libevent liblzf libpcap mdocml \
	ntp openresolv tcpdump tmux top tre zstd

.if (${MKATF} != "no")
SUBDIR+= atf
.endif

.if (${MKFIDO2} != "no")
SUBDIR+= libfido2
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

.if ${MKWLAN} != "no"
SUBDIR+= wpa
.endif

SUBDIR+= blocklist pdisk nvi ppp libnv tradcpp

.include <bsd.subdir.mk>
