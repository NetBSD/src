#	$NetBSD: Makefile,v 1.171 2002/03/22 18:10:24 thorpej Exp $
#	from: @(#)Makefile	5.20 (Berkeley) 6/12/93

.include <bsd.own.mk>

SUBDIR=	ac accton altq amd apm apmd arp bad144 bind bootp catman \
	chown chroot chrtbl cnwctl config cron dev_mkdb \
	dhcp diskpart dumpfs dumplfs edquota eeprom \
	envstat eshconfig etcupdate greconfig grfconfig \
	grfinfo gspa hilinfo ifwatchd inetd iopctl iostat ipf \
	irdaattach isdn iteconfig \
	kgmon kvm_mkdb lastlogin link lpr mailwrapper makefs map-mbone \
	mdconfig memswitch mlxctl mopd mountd moused mrinfo mrouted mtrace \
	mtree ndbootd netgroup_mkdb nfsd ntp pcictl pkg_install pppd pstat \
	pwd_mkdb quot quotacheck quotaon rarpd rbootd rdate \
	repquota rmt rpc.bootparamd rpc.lockd rpc.pcnfsd \
	rpc.statd rpcbind rwhod sa screenblank sesd \
	sliplogin slstats spray sup sushi syslogd tadpolectl tcpdchk \
	tcpdmatch tcpdump timed traceroute trpt trsp unlink \
	usbdevs user videomode vipw vnconfig wiconfig wsconscfg \
	wsfontload wsmuxctl zdump zic

.if (${MKKERBEROS} != "no")
SUBDIR+= hprop kadmin kdc kstash ktutil
.endif

.if (${MKYP} != "no")
SUBDIR+= rpc.yppasswdd ypbind yppoll ypserv ypset
.endif

.if !defined(USE_NEW_TOOLCHAIN)
SUBDIR+= mdsetimage
.endif

# IPv6
SUBDIR+=faithd ifmcstat mld6query ndp \
	rip6query route6d rtadvd traceroute6 rtsold

# ATM PVC
SUBDIR+=pvcsif pvctxctl

.if (${MKCRYPTO} != "no")
SUBDIR+= racoon
.endif

.include <bsd.subdir.mk>
