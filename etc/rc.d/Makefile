# $NetBSD: Makefile,v 1.2 2000/03/13 04:03:53 lukem Exp $

FILES=	DAEMON LOGIN SERVERS accounting amd apmd bootparams bootconf.sh ccd \
	cleartmp cron dhclient dhcpd dhcrelay dmesg fsck.sh gated inetd \
	ipfilter ipmon ipnat kerberos ldconfig lkm1 lkm2 lkm3 local lpd mopd \
	motd mountall mountcritlocal mountcritremote mountd mrouted named \
	network nfsd nfsiod nfslocking ntpdate portmap ppp pwcheck quota \
	raidframe rarpd rbootd root route6d routed rtadvd rtsold rwho savecore \
	screenblank sendmail swap1 swap2 sysdb sysctl syslogd systemfs timed \
	ttys virecover wscons xdm xfs xntpd ypbind yppasswdd ypserv
FILESDIR=	/etc/rc.d
FILESMODE=	${BINMODE}

NOPROG= noprog

.include <bsd.prog.mk>
