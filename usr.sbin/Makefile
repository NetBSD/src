#	from: @(#)Makefile	5.6.1.2 (Berkeley) 5/8/91
#	$Id: Makefile,v 1.32 1994/04/04 21:49:39 cgd Exp $

# not yet done: catman

SUBDIR=	ac accton amd arp chown chroot config config.new cron dev_mkdb \
	diskpart edquota gettable htable inetd iostat kgmon \
	kvm_mkdb lpr mrouted mtree named portmap pppd pwd_mkdb quot quotaon \
	rarpd rbootd repquota rmt rpc.bootparamd rwhod \
	sa sendmail sliplogin slstats spray swapinfo \
	syslogd tcpdump timed traceroute trpt trsp update vipw vnconfig \
	ypbind yppoll ypset

.if   make(clean) || make(cleandir)
SUBDIR+=bad144 hilinfo grfinfo
.elif ${MACHINE} == "i386"
SUBDIR+=bad144 
.elif ${MACHINE} == "hp300"
# probably could be expanded to other machines, too
SUBDIR+=hilinfo grfinfo
.endif

.include <bsd.subdir.mk>
