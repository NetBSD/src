#	$NetBSD: Makefile,v 1.30.24.1 2013/02/25 00:27:46 tls Exp $

.include <bsd.own.mk>

#
# Makefile for building a standalone send-pr.
#

srcdir=		${.CURDIR}
bindir=		/usr/bin
datadir=	/usr/share

#
# Begin CONFIGURATION SECTION
#
VERSION=3.95

# Where the GNATS database and master configuration stuff lives.
GNATS_ROOT =

# Email address to which people should submit bugs.
GNATS_ADDR = gnats-bugs@NetBSD.org

# The default release for this send-pr.
DEFAULT_RELEASE = `uname -s` `uname -r`

# The default organization for this send-pr.
DEFAULT_ORGANIZATION =

# The default GNATS site for send-pr.
GNATS_SITE = netbsd

# The submitter-id for your site.
SUBMITTER = net

# Command line to use for mailer
MAIL_AGENT = /usr/sbin/sendmail -oi -t

# Command line to display the passwd database
PASSWD = cat /etc/passwd

# Echo(1) style
ECHON = bsd

#
# END OF CONFIGURATION SECTION
#

BINDIR=		${bindir}
LIBDIR=		${datadir}
MAN=		send-pr.1

CLEANFILES=	send-pr send-pr.1 send-pr.el install-sid

realall:	send-pr send-pr.1 send-pr.el install-sid

send-pr:	send-pr.sh Makefile
	${_MKTARGET_CREATE}
	${TOOL_SED} -e 's,xVERSIONx,$(VERSION),' \
            -e 's,xGNATS_ROOTx,$(GNATS_ROOT),' \
            -e 's,xGNATS_ADDRx,$(GNATS_ADDR),' \
            -e 's,xGNATS_SITEx,$(GNATS_SITE),' \
            -e 's,xSUBMITTERx,$(SUBMITTER),' \
            -e 's,xECHONx,$(ECHON),' \
            -e 's,xMAIL_AGENTx,$(MAIL_AGENT),' \
            -e 's,xPASSWDx,$(PASSWD),' \
	    -e 's%xDEFAULT_ORGANIZATIONx%$(DEFAULT_ORGANIZATION)%' \
	    -e 's,xDEFAULT_RELEASEx,$(DEFAULT_RELEASE),' \
	    -e 's,xDATADIRx,$(datadir),' $(srcdir)/send-pr.sh > $@-t
	mv -f $@-t $@
	chmod 755 $@

install-sid:	install-sid.sh Makefile
	${_MKTARGET_CREATE}
	${TOOL_SED} -e 's,xBINDIRx,$(bindir),g' \
	    -e 's,xVERSIONx,$(VERSION),g' $(srcdir)/install-sid.sh > $@-t
	mv -f $@-t $@
	chmod 755 $@

send-pr.el:	send-pr-el.in Makefile
	${_MKTARGET_CREATE}
	${TOOL_SED} -e 's,xVERSIONx,$(VERSION),' \
            -e 's,xGNATS_ROOTx,$(GNATS_ROOT),' \
            -e 's,xGNATS_SITEx,$(GNATS_SITE),' \
            -e 's,xDATADIRx,$(datadir),' \
            -e 's,xSUBMITTERx,$(SUBMITTER),' \
            -e 's,xDEFAULT_RELEASEx,$(DEFAULT_RELEASE),' \
		$(srcdir)/send-pr-el.in \
            > $@-t
	mv -f $@-t $@

send-pr.1:	send-pr.man Makefile
	${_MKTARGET_CREATE}
	${TOOL_SED} -e 's,xGNATS_ROOTx,$(GNATS_ROOT),g' \
	    -e 's,xVERSIONx,$(VERSION),g' $(srcdir)/send-pr.man > $@-t
	mv -f $@-t $@

FILES=send-pr install-sid
FILESMODE_send-pr=${BINMODE}
FILESMODE_install-sid=${BINMODE}
.if ${MKSHARE} != "no"
FILES+=categories send-pr.el
FILESDIR_send-pr.el=${LIBDIR}/gnats
FILESDIR_categories=${LIBDIR}/gnats
FILESNAME_categories=${GNATS_SITE}
.endif

TEXINFO=	send-pr.texi
INFOFLAGS=	-I${.CURDIR}

.include <bsd.info.mk>
.include <bsd.prog.mk>
