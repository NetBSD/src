#	@(#)Makefile	5.6.1.2 (Berkeley) 5/8/91
#
#	$Header: /cvsroot/src/usr.sbin/Makefile,v 1.7 1993/04/10 12:16:41 cgd Exp $
#

SUBDIR=	accton arp chroot config dbsym dev_mkdb diskpart \
	edquota flcopy gettable htable inetd iostat kgmon \
	kvm_mkdb lpr mtree named portmap pwd_mkdb quotaon repquota \
	rmt rwalld rwhod sendmail sliplogin swapinfo syslogd timed \
	traceroute trpt trsp update vipw

.if   make(clean) || make(cleandir)
SUBDIR+=bad144
.elif ${MACHINE} == "i386"
SUBDIR+=bad144 
.endif

.include <bsd.subdir.mk>
