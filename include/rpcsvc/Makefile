#
# @(#)Makefile	2.3 88/08/11 4.0 RPCSRC
#
# $Header: /cvsroot/src/include/rpcsvc/Attic/Makefile,v 1.1 1993/04/04 19:00:06 cgd Exp $
#

.SUFFIXES: .x

RPCCOM = rpcgen

HDRS= klm_prot.h mount.h nfs_prot.h nlm_prot.h rex.h rquota.h rnusers.h\
      rquota.h rstat.h rwall.h sm_inter.h spray.h yppasswd.h yp.h
XFILES= bootparam_prot.x klm_prot.x mount.x nfs_prot.x nlm_prot.x \
      rex.x rnusers.x rquota.x rstat.x rwall.x sm_inter.x spray.x \
      yppasswd.x yp.x

CLEANFILES+= ${HDRS}

all:	${HDRS}

install:
	@echo "Creating RPC service headers directory"
	@/bin/rm -rf ${DESTDIR}/usr/include/rpcsvc
	@-mkdir ${DESTDIR}/usr/include/rpcsvc && \
		chown bin ${DESTDIR}/usr/include/rpcsvc && \
		chmod 755 ${DESTDIR}/usr/include/rpcsvc
	@echo "Installing RPC service header and definition files"
	@for i in $(XFILES); do \
		(install -c -m 644 ${.CURDIR}/$$i ${DESTDIR}/usr/include/rpcsvc) done
	@for i in $(HDRS); do \
		(install -c -m 644 $$i ${DESTDIR}/usr/include/rpcsvc) done

.x.h:
	@echo generating $@...
	@PWD=`pwd` ; cd ${.CURDIR} ; ${RPCCOM} -h $*.x -o $$PWD/$@

.include <bsd.prog.mk>
