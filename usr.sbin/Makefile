#	$NetBSD: Makefile,v 1.214 2006/06/19 15:44:56 gdamore Exp $
#	from: @(#)Makefile	5.20 (Berkeley) 6/12/93

.include <bsd.own.mk>

SUBDIR=	ac accton altq amd apm apmd arp bad144 bind bootp \
	btconfig btcontrol bthcid catman \
	chown chroot chrtbl cnwctl cron dev_mkdb \
	dhcp diskpart dumpfs dumplfs edquota eeprom \
	envstat eshconfig etcupdate extattrctl fssconfig fwctl gpioctl \
	grfconfig grfinfo gspa hilinfo ifwatchd inetd installboot \
	iopctl iostat ipwctl irdaattach iscsi isdn iteconfig iwictl\
	kgmon lastlogin link lmcconfig lpr mailwrapper makefs \
	map-mbone mdconfig memswitch mlxctl mopd mountd moused \
	mrinfo mrouted mscdlabel mtrace \
	mtree ndbootd ndiscvt netgroup_mkdb nfsd ntp pcictl pkg_install pppd \
	pstat pwd_mkdb postinstall powerd quot quotacheck quotaon \
	rarpd rbootd rdate \
	repquota rmt rpc.bootparamd rpc.lockd rpc.pcnfsd \
 	rpc.statd rpcbind rwhod sa screenblank sdpd sesd \
	sliplogin sntp \
	spray sti sunlabel sup syslogd tadpolectl tcpdchk \
	tcpdmatch tcpdump timed tpctl traceroute trpt trsp unlink \
	usbdevs user videomode vipw vnconfig wiconfig wlanctl wpa \
	wsconscfg wsfontload wsmoused wsmuxctl zdump zic

.if (${MKKERBEROS} != "no")
SUBDIR+= hprop kadmin kdc kstash ktutil
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
