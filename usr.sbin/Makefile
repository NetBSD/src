#	$NetBSD: Makefile,v 1.158 2001/06/02 00:34:00 itojun Exp $
#	from: @(#)Makefile	5.20 (Berkeley) 6/12/93

.include <bsd.own.mk>

SUBDIR=	ac accton altq amd apm apmd arp bad144 bind bootp catman \
	chown chroot chrtbl cnwctl config cron dbsym dev_mkdb \
	dhcp diskpart dumpfs dumplfs edquota eeprom \
	envstat eshconfig greconfig \
	grfconfig grfinfo gspa hilinfo inetd iopctl iostat ipf isdn iteconfig \
	kgmon kvm_mkdb lastlogin link lpr mailwrapper map-mbone \
	mdconfig mdsetimage \
	memswitch mlxctl mopd mountd mrinfo mrouted mtrace mtree \
	ndbootd netgroup_mkdb nfsd ntp pkg_install pppd pstat \
	pwd_mkdb quot quotacheck quotaon rarpd rbootd rdate \
	repquota rmt rpc.bootparamd rpc.lockd rpc.pcnfsd \
	rpc.statd rpc.yppasswdd rpcbind rwhod sa screenblank sesd \
	sliplogin slstats spray sup sushi syslogd tadpolectl tcpdchk \
	tcpdmatch tcpdump timed traceroute trpt trsp unlink \
	usbdevs user videomode vipw vnconfig wiconfig wsconscfg \
	wsfontload ypbind yppoll ypserv ypset zdump zic

# IPv6
SUBDIR+=faithd ifmcstat mld6query mtrace6 ndp \
	rip6query route6d rtadvd traceroute6 rtsold

# ATM PVC
SUBDIR+=pvcsif pvctxctl

.if (${MKCRYPTO} != "no")
SUBDIR+= hprop kadmin kdc kstash ktutil racoon
.endif

.include <bsd.subdir.mk>
