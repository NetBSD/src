#	$NetBSD: Makefile,v 1.238.2.1 2009/05/13 19:20:14 jym Exp $
#	from: @(#)Makefile	5.20 (Berkeley) 6/12/93

.include <bsd.own.mk>

SUBDIR=	ac accton acpitools altq apm apmd arp bad144 bootp \
	btattach btconfig btdevctl bthcid btpand catman \
	chown chroot chrtbl cnwctl cpuctl crash cron dev_mkdb \
	dhcp diskpart dumpfs dumplfs edquota eeprom \
	envstat eshconfig etcupdate extattrctl fssconfig fusermount fwctl \
	gpioctl grfconfig grfinfo gspa hilinfo ifwatchd inetd installboot \
	iopctl iostat ipwctl irdaattach isdn iteconfig iwictl\
	kgmon lastlogin link lmcconfig lockstat lpr mailwrapper makefs \
	map-mbone mdconfig memswitch mlxctl mmcformat mopd mountd moused \
	mrinfo mrouted mscdlabel mtrace \
	mtree ndbootd ndiscvt netgroup_mkdb nfsd ntp ofctl paxctl pcictl \
	pppd psrset pstat pwd_mkdb postinstall powerd puffs quot quotacheck \
	quotaon rarpd rbootd rdate \
	repquota rmt rpc.bootparamd rpc.lockd rpc.pcnfsd \
 	rpc.statd rpcbind rwhod sa screenblank sdpd services_mkdb sesd \
	schedctl sliplogin sntp \
	spray srtconfig sti sunlabel sup syslogd tadpolectl tcpdchk \
	tcpdmatch tcpdump tcpdrop timed tpctl traceroute trpt unlink \
	usbdevs user videomode vipw veriexecgen vnconfig wiconfig wlanctl wpa \
	wsconscfg wsfontload wsmoused wsmuxctl zdump zic

.if (${MKISCSI} != "no")
SUBDIR+= iscsi 
.endif

.if (${MKKERBEROS} != "no")
SUBDIR+= hprop kadmin kdc kstash ktutil iprop-log kdigest kimpersonate kcm
.endif

.if (${MKYP} != "no")
SUBDIR+= rpc.yppasswdd ypbind yppoll ypserv ypset
.endif

.if ${TOOLCHAIN_MISSING} != "no"
SUBDIR+= mdsetimage
.endif

.if (${MKINET6} != "no")
SUBDIR+=faithd ifmcstat ndp rip6query rtsold
.endif
.if (${USE_INET6} != "no")
SUBDIR+=mld6query route6d rtadvd traceroute6 
.endif

# ATM PVC
SUBDIR+=pvcsif pvctxctl

.if (${MKCRYPTO} != "no")
SUBDIR+= racoon racoonctl
.endif

# IP Filter
.if (${MKIPFILTER} != "no")
SUBDIR+=ipf
.endif

# PF
.if (${MKPF} != "no")
SUBDIR+=pf
.endif

.include <bsd.subdir.mk>
