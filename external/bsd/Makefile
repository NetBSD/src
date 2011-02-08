#	$NetBSD: Makefile,v 1.26.2.1 2011/02/08 16:18:50 bouyer Exp $

.include <bsd.own.mk>

SUBDIR+= am-utils bind cron dhcpcd fetch file flex libarchive libevent liblzf \
	libpcap mdocml openresolv top byacc wpa ntp tcpdump

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
