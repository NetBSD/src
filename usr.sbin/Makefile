#	$NetBSD: Makefile,v 1.89.2.1 1998/11/23 06:40:11 cgd Exp $
#	from: @(#)Makefile	5.20 (Berkeley) 6/12/93
#
# not yet done: catman

# XXX Temporary for NO_SENDMAIL
.include <bsd.own.mk>

SUBDIR=	ac accton amd apm apmd arp bad144 bootpd bootpef bootpgw bootptest \
	chown chroot chrtbl config cron dbsym dev_mkdb dhcp diskpart dumpfs \
	dumplfs edquota eeprom gettable grfconfig grfinfo gspa hilinfo htable \
	inetd iostat ipf iteconfig kgmon kvm_mkdb lastlogin lpr map-mbone \
	mdconfig mdsetimage mopd mountd mrinfo mrouted mtrace mtree named \
	netgroup_mkdb nfsd nfsiod pkg_install portmap pppd pstat pwd_mkdb quot \
	quotacheck quotaon rarpd rbootd rdate repquota rmt rpc.bootparamd \
	rpc.lockd rpc.pcnfsd rpc.statd rpc.yppasswdd rwhod sa screenblank \
	sliplogin slstats spray sup sysctl syslogd tcpdchk tcpdmatch \
	tcpdump timed traceroute trpt trsp update videomode vipw vnconfig xntp \
	ypbind yppoll ypserv ypset zdump zic

.ifndef NO_SENDMAIL
SUBDIR+=        sendmail
.endif

.include <bsd.subdir.mk>
