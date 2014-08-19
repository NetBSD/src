# $NetBSD: Makefile,v 1.5.2.1 2014/08/19 23:45:52 tls Exp $

.if (${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64")
PROG=	iasl
.endif
BINDIR=	/usr/bin
MAN=	iasl.8
WARNS=	0
TOPDIR=	${NETBSDSRCDIR}/sys/external/bsd/acpica/dist/compiler

.include <bsd.own.mk>

CPPFLAGS+=	-I${TOPDIR}/../include -I${TOPDIR} -I.
CPPFLAGS+=	-D__NetBSD__ -DACPI_ASL_COMPILER -DACPI_USE_ALTERNATE_TIMEOUT
CPPFLAGS+=	-D_USE_BERKELEY_YACC
.if ${MKREPRO:Uno} == "yes"
CPPFLAGS+=	-DACPI_REPRO
.endif
LDADD+=		-ll -ly -lrt -lpthread
DPADD+=		${LIBL} ${LIBY} ${LIBRT} ${LIBPTHREAD}

DPSRCS= aslcompilerparse.c aslcompilerlex.c \
	dtparserparse.c dtparserlex.c prparserparse.c prparserlex.c

.PATH:	${TOPDIR}
SRCS+= \
	adfile.c \
	aslanalyze.c \
	aslbtypes.c \
	aslcodegen.c \
	aslcompile.c \
	aslcompilerlex.c \
	aslcompilerparse.c \
	aslerror.c \
	aslfileio.c \
	aslfiles.c \
	aslfold.c \
	aslhex.c \
	asllength.c \
	asllisting.c \
	asllistsup.c \
	aslload.c \
	asllookup.c \
	aslmain.c \
	aslmap.c \
	aslmethod.c \
	aslnamesp.c \
	asloffset.c \
	aslopcodes.c \
	asloperands.c \
	aslopt.c \
	asloptions.c \
	aslpredef.c \
	aslprepkg.c \
	aslresource.c \
	aslrestype1.c \
	aslrestype1i.c \
	aslrestype2.c \
	aslrestype2d.c \
	aslrestype2e.c \
	aslrestype2q.c \
	aslrestype2s.c \
	aslrestype2w.c \
	aslstartup.c \
	aslstubs.c \
	asltransform.c \
	asltree.c \
	aslutils.c \
	asluuid.c \
	aslwalks.c \
	aslxref.c \
	dtcompile.c \
	dtexpress.c \
	dtfield.c \
	dtio.c \
	dtparserlex.c \
	dtparserparse.c \
	dtsubtable.c \
	dttable.c \
	dttemplate.c \
	dtutils.c \
	prexpress.c \
	prmacros.c \
	prparserlex.c \
	prparserparse.c \
	prscan.c \
	prutils.c

.PATH: ${TOPDIR}/../common
SRCS+= \
	adisasm.c \
	adwalk.c \
	ahpredef.c \
	dmextern.c \
	dmrestag.c \
	dmtable.c \
	dmtbdump.c \
	dmtbinfo.c \
	getopt.c

.PATH: ${TOPDIR}/../debugger
SRCS+=  dbfileio.c

.PATH: ${TOPDIR}/../disassembler
SRCS+= \
	dmbuffer.c \
	dmdeferred.c \
	dmnames.c \
	dmobject.c \
	dmopcode.c \
	dmresrc.c \
	dmresrcl.c \
	dmresrcl2.c \
	dmresrcs.c \
	dmutils.c \
	dmwalk.c

.PATH: ${TOPDIR}/../dispatcher
SRCS+= \
	dsargs.c \
	dscontrol.c \
	dsfield.c \
	dsobject.c \
	dsopcode.c \
	dsutils.c \
	dswexec.c \
	dswload.c \
	dswload2.c \
	dswscope.c \
	dswstate.c

.PATH: ${TOPDIR}/../executer
SRCS+= \
	exconvrt.c \
	excreate.c \
	exdump.c \
	exmisc.c \
	exmutex.c \
	exnames.c \
	exoparg1.c \
	exoparg2.c \
	exoparg3.c \
	exoparg6.c \
	exprep.c \
	exregion.c \
	exresnte.c \
	exresolv.c \
	exresop.c \
	exstore.c \
	exstoren.c \
	exstorob.c \
	exsystem.c \
	exutils.c

.PATH: ${TOPDIR}/../parser
SRCS+= \
	psargs.c \
	psloop.c \
	psobject.c \
	psopcode.c \
	psopinfo.c \
	psparse.c \
	psscope.c \
	pstree.c \
	psutils.c \
	pswalk.c

.PATH: ${TOPDIR}/../tables
SRCS+=	\
	tbfadt.c \
	tbinstal.c \
	tbprint.c \
	tbutils.c \
	tbxface.c

.PATH: ${TOPDIR}/../namespace
SRCS+= \
	nsaccess.c \
	nsalloc.c \
	nsdump.c \
	nsnames.c \
	nsobject.c \
	nsparse.c \
	nssearch.c \
	nsutils.c \
	nswalk.c \
	nsxfobj.c \

.PATH: ${TOPDIR}/../utilities
SRCS+= \
	utaddress.c \
	utalloc.c \
	utbuffer.c \
	utcache.c \
	utcopy.c \
	utdebug.c \
	utdecode.c \
	utdelete.c \
	uterror.c \
	utexcep.c \
	utglobal.c \
	utinit.c \
	utlock.c \
	utmath.c \
	utmisc.c \
	utmutex.c \
	utobject.c \
	utosi.c \
	utownerid.c \
	utpredef.c \
	utresrc.c \
	utstate.c \
	utstring.c \
	utxface.c \
	utxferror.c

.PATH: ${TOPDIR}/../os_specific/service_layers
SRCS+=	osunixxf.c

aslcompiler.y.h aslcompilerparse.c: ${TOPDIR}/aslcompiler.y
	${YACC} -v -d -o${.TARGET} -pAslCompiler ${.ALLSRC}
	mv -f ${.PREFIX}.h aslcompiler.y.h

aslcompilerlex.c: ${TOPDIR}/aslcompiler.l
	${LEX} -i -PAslCompiler -o${.TARGET} ${.ALLSRC}

aslcompilerlex.o: aslcompiler.y.h

dtparser.y.h dtparserparse.c: ${TOPDIR}/dtparser.y
	${YACC} -v -d -o${.TARGET} -pDtParser ${.ALLSRC}
	mv -f ${.PREFIX}.h dtparser.y.h

dtparserlex.c: ${TOPDIR}/dtparser.l
	${LEX} -i -PDtParser -o${.TARGET} ${.ALLSRC}

dtparserlex.o: dtparser.y.h

prparser.y.h prparserparse.c: ${TOPDIR}/prparser.y
	${YACC} -v -d -o${.TARGET} -pPrParser ${.ALLSRC}
	mv -f ${.PREFIX}.h prparser.y.h

prparserlex.c: ${TOPDIR}/prparser.l
	${LEX} -i -PPrParser -o${.TARGET} ${.ALLSRC}

prparserlex.o: prparser.y.h

CLEANFILES+=	aslcompilerlex.c aslcompilerparse.c	\
		aslcompiler.y.h aslcompilerparse.output \
		dtparserlex.c dtparserparse.c		\
		dtparser.y.h dtparserparse.output	\
		prparserlex.c prparserparse.c		\
		prparser.y.h prparserparse.output	\

.include <bsd.prog.mk>
