#	$NetBSD: Makefile,v 1.146 2000/06/30 18:02:32 thorpej Exp $
#	from: @(#)Makefile	5.20 (Berkeley) 6/12/93

.include <bsd.own.mk>

SUBDIR=	ac accton amd apm apmd arp bad144 bind bootp catman \
	chown chroot chrtbl cnwctl config cron dbsym dev_mkdb \
	dhcp diskpart dumpfs dumplfs edquota eeprom \
	envstat eshconfig greconfig \
	grfconfig grfinfo gspa hilinfo inetd iostat ipf iteconfig kgmon \
	kvm_mkdb lastlogin link lpr mailwrapper map-mbone \
	mdconfig mdsetimage \
	memswitch mopd mountd mrinfo mrouted mtrace mtree \
	netgroup_mkdb nfsd ntp pkg_install pppd pstat \
	pwd_mkdb quot quotacheck quotaon rarpd rbootd rdate \
	repquota rmt rpc.bootparamd rpc.lockd rpc.pcnfsd \
	rpc.statd rpc.yppasswdd rpcbind rwhod sa screenblank sesd \
	sliplogin slstats spray sup syslogd tadpolectl tcpdchk \
	tcpdmatch tcpdump timed traceroute trpt trsp unlink \
	usbdevs user videomode vipw vnconfig wiconfig wsconscfg \
	wsfontload ypbind yppoll ypserv ypset zdump zic

# IPv6
SUBDIR+=faithd ifmcstat mld6query mtrace6 ndp pim6dd pim6sd \
	rip6query route6d rtadvd traceroute6 rtsold

# ATM PVC
SUBDIR+=pvcsif pvctxctl

.if (${MKCRYPTO} != "no")
SUBDIR+= kadmin kstash ktutil
.endif

.include <bsd.subdir.mk>
