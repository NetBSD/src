#	$NetBSD: Makefile,v 1.116 1999/07/02 17:41:23 itojun Exp $
#	from: @(#)Makefile	5.20 (Berkeley) 6/12/93

# XXX Temporary for NO_SENDMAIL and BUILD_POSTFIX
.include <bsd.own.mk>

SUBDIR=	ac accton amd apm apmd arp bad144 bind bootp catman \
	chown chroot chrtbl config cron dbsym dev_mkdb \
	dhcp diskpart dumpfs dumplfs edquota eeprom eshconfig greconfig \
	grfconfig grfinfo gspa hilinfo inetd iostat ipf iteconfig kgmon \
	kvm_mkdb lastlogin lpr mailwrapper map-mbone mbrlabel \
	mdconfig mdsetimage \
	memswitch mopd mountd mrinfo mrouted mtrace mtree \
	netgroup_mkdb nfsd nfsiod pkg_install portmap pppd pstat \
	pwd_mkdb quot quotacheck quotaon rarpd rbootd rdate \
	repquota rmt rpc.bootparamd rpc.lockd rpc.pcnfsd \
	rpc.statd rpc.yppasswdd rwhod sa screenblank \
	sliplogin slstats spray sup sysctl syslogd tcpdchk \
	tcpdmatch tcpdump timed traceroute trpt trsp update \
	usbdevs videomode vipw vnconfig wsconscfg wsfontload xntp \
	ypbind yppoll ypserv ypset \
	zdump zic

# IPv6
SUBDIR+=gifconfig ifmcstat ndp rip6query route6d rtadvd setkey traceroute6

.if !defined(NO_SENDMAIL)
SUBDIR+=	sendmail
.endif

.if defined(BUILD_POSTFIX)
SUBDIR+=	postfix
.endif

.include <bsd.subdir.mk>
