#	from: @(#)Makefile	5.6.1.2 (Berkeley) 5/8/91
#	$Id: Makefile,v 1.27 1994/02/07 23:31:18 cgd Exp $

# not yet done: catman

SUBDIR=	accton amd arp chown chroot config config.new cron dev_mkdb \
	diskpart edquota gettable htable inetd iostat kgmon \
	kvm_mkdb lpr mrouted mtree named portmap pppd pwd_mkdb quotaon \
	rarpd rbootd repquota rmt rpc.bootparamd rwhod \
	sendmail sliplogin slstats spray swapinfo \
	syslogd tcpdump timed traceroute trpt trsp update vipw vnconfig yp

.if   make(clean) || make(cleandir)
SUBDIR+=bad144
.elif ${MACHINE} == "i386"
SUBDIR+=bad144 
.endif

.include <bsd.subdir.mk>
