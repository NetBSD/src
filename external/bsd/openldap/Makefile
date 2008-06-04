#	$NetBSD: Makefile,v 1.1.2.2 2008/06/04 02:03:28 yamt Exp $

.include "openldap.mk"

SUBDIR=		include

SUBDIR+=	lib .WAIT

SUBDIR+=	bin


#
#	maintainer rules
#	----------------
#
#	configure	run configure and create various files for target build.
#	datfiles	rebuild the datfiles
#

#
#	Run configure to create various files.
#	This should only be necessary after updating ./dist/
#

CONFIGURE_ARGS+=	--prefix=${LDAP_PREFIX}
CONFIGURE_ARGS+=	--sysconfdir=${LDAP_ETCDIR}
CONFIGURE_ARGS+=	--localstatedir=${LDAP_RUNDIR}

CONFIGURE_ARGS+=	--enable-dynamic
CONFIGURE_ARGS+=	--disable-slapd
CONFIGURE_ARGS+=	--with-tls=openssl
CONFIGURE_ARGS+=	--with-threads
CONFIGURE_ARGS+=	--without-cyrus-sasl

configure: work/config.status .PHONY 

work/config.status: ${LDAP_DISTDIR}/configure Makefile openldap.mk
	mkdir -p work
	(cd work \
	    && ${CONFIGURE_ENV} sh ${LDAP_DISTDIR}/configure ${CONFIGURE_ARGS} \
	    || false)

update-include: work/config.status .PHONY
	(cd work/include \
	    && ${MAKE} ldap_config.h \
	    || false)
	@for wf in `find work/include -name '*.h'`; do \
		tf=${LDAP_SRCDIR}/include/$${wf##*/}; \
		tf=$${tf%.tmp} ; \
		cmp -s $${wf} $${tf} > /dev/null 2>&1 || ( \
			rm -f $${tf} && \
			cp $${wf} $${tf} && \
			echo "Updated $${tf}" ; \
		) ; \
	done

update-man: work/config.status .PHONY
	(cd work/doc/man \
	    && ${MAKE} \
	    || false)
	@for wf in `find work/doc/man -name '*.[0-9].tmp'` ; do \
		tf=${LDAP_SRCDIR}/man/$${wf##*/}; \
		tf=$${tf%.tmp} ; \
		cmp -s $${wf} $${tf} > /dev/null 2>&1 || ( \
			rm -f $${tf} && \
			cp $${wf} $${tf} && \
			echo "Updated $${tf}" ; \
		) ; \
	done


.PHONY: clean.work

clean: clean.work
clean.work:
	-rm -f -r work


.include <bsd.hostprog.mk>
.include <bsd.subdir.mk>
