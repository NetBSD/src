#	@(#)Makefile	8.19 (Berkeley) 1/14/97

#
#  This Makefile uses the new Berkeley "make" program.  See Makefile.dist
#  for a more vanilla version.
#
#  Create configuration files using "m4 ../m4/cf.m4 file.mc > file.cf";
#  this may be easier than tweaking the Makefile.  You do need to
#  have a fairly modern M4 available (GNU m4 works).  On SunOS, use
#  /usr/5bin/m4.
#

M4=	m4
CFDIR=	..
CHMOD=	chmod
ROMODE=	444
RM=	rm -f

.SUFFIXES:  .mc .cf

.mc.cf:
	$(RM) $@
	(cd ${.CURDIR} && $(M4) ${@:R}.mc > ${.OBJDIR}/$@)
	$(CHMOD) $(ROMODE) $@

ALL=	boat-anchor.cf chimera.cf sun-lamp.cf trinity.cf \
	clientproto.cf netbsd-proto.cf tcpproto.cf uucpproto.cf

all: $(ALL)

clean cleandir:
	$(RM) $(ALL) core

depend install: 

distribution:
	${INSTALL} -c -o root -g wheel -m 444 netbsd-proto.cf \
	    ${DESTDIR}/etc/sendmail.cf

# this is overkill, but....
M4FILES=\
	${CFDIR}/domain/Berkeley.EDU.m4 \
	${CFDIR}/domain/CS.Berkeley.EDU.m4 \
	${CFDIR}/domain/EECS.Berkeley.EDU.m4 \
	${CFDIR}/domain/S2K.Berkeley.EDU.m4 \
	${CFDIR}/feature/allmasquerade.m4 \
	${CFDIR}/feature/always_add_domain.m4 \
	${CFDIR}/feature/bestmx_is_local.m4 \
	${CFDIR}/feature/bitdomain.m4 \
	${CFDIR}/feature/domaintable.m4 \
	${CFDIR}/feature/local_procmail.m4 \
	${CFDIR}/feature/mailertable.m4 \
	${CFDIR}/feature/nocanonify.m4 \
	${CFDIR}/feature/nodns.m4 \
	${CFDIR}/feature/notsticky.m4 \
	${CFDIR}/feature/nouucp.m4 \
	${CFDIR}/feature/nullclient.m4 \
	${CFDIR}/feature/redirect.m4 \
	${CFDIR}/feature/smrsh.m4 \
	${CFDIR}/feature/stickyhost.m4 \
	${CFDIR}/feature/use_cw_file.m4 \
	${CFDIR}/feature/uucpdomain.m4 \
	${CFDIR}/hack/cssubdomain.m4 \
	${CFDIR}/m4/cf.m4 \
	${CFDIR}/m4/cfhead.m4 \
	${CFDIR}/m4/nullrelay.m4 \
	${CFDIR}/m4/proto.m4 \
	${CFDIR}/m4/version.m4 \
	${CFDIR}/mailer/cyrus.m4 \
	${CFDIR}/mailer/fax.m4 \
	${CFDIR}/mailer/local.m4 \
	${CFDIR}/mailer/mail11.m4 \
	${CFDIR}/mailer/pop.m4 \
	${CFDIR}/mailer/procmail.m4 \
	${CFDIR}/mailer/smtp.m4 \
	${CFDIR}/mailer/usenet.m4 \
	${CFDIR}/mailer/uucp.m4 \
	${CFDIR}/ostype/bsd4.4.m4 \
	${CFDIR}/siteconfig/uucp.cogsci.m4 \
	${CFDIR}/siteconfig/uucp.old.arpa.m4 \
	${CFDIR}/siteconfig/uucp.ucbarpa.m4 \
	${CFDIR}/siteconfig/uucp.ucbvax.m4 \

$(ALL):  $(M4FILES)

.include <bsd.prog.mk>
