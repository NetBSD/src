#	@(#)Makefile	5.6.1.2 (Berkeley) 5/8/91
#
#	$Header: /cvsroot/src/usr.sbin/Makefile,v 1.5 1993/04/05 03:40:11 cgd Exp $
#

SUBDIR=	arp chroot config dbsym dev_mkdb diskpart \
	inetd kgmon kvm_mkdb lpr \
	mtree named portmap pwd_mkdb rmt rwalld rwhod\
	sendmail sliplogin syslogd timed traceroute update vipw

.if   make(clean) || make(cleandir)
SUBDIR+=bad144
.elif ${MACHINE} == "i386"
SUBDIR+=bad144 
.endif

.include <bsd.subdir.mk>
