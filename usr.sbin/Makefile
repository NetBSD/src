#	$NetBSD: Makefile,v 1.97 1998/11/02 03:32:03 kim Exp $
#	from: @(#)Makefile	5.20 (Berkeley) 6/12/93
#
# not yet done: catman

# XXX Temporary for NO_SENDMAIL and USE_BIND8
.include <bsd.own.mk>

SUBDIR=	ac accton amd apm apmd arp bad144 bootp \
	chown chroot chrtbl config cron dbsym dev_mkdb \
	dhcp diskpart dumpfs dumplfs edquota eeprom eshconfig greconfig \
	grfconfig grfinfo gspa hilinfo inetd iostat ipf iteconfig kgmon \
	kvm_mkdb lastlogin lpr map-mbone mdconfig mdsetimage \
	mopd mountd mrinfo mrouted mtrace mtree \
	netgroup_mkdb nfsd nfsiod pkg_install portmap pppd pstat \
	pwd_mkdb quot quotacheck quotaon rarpd rbootd rdate \
	repquota rmt rpc.bootparamd rpc.lockd rpc.pcnfsd \
	rpc.statd rpc.yppasswdd rwhod sa screenblank \
	sliplogin slstats spray sup sysctl syslogd tcpdchk \
	tcpdmatch tcpdump timed traceroute trpt trsp update \
	usbd usbdevs videomode vipw vnconfig xntp \
	ypbind yppoll ypserv ypset \
	zdump zic

.ifndef NO_SENDMAIL
SUBDIR+=	sendmail
.endif

.ifdef USE_BIND8
SUBDIR+=	bind
.else
SUBDIR+=	named
.endif

.include <bsd.subdir.mk>
