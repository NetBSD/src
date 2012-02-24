# $NetBSD: Makefile,v 1.4.4.1 2012/02/24 17:18:58 riz Exp $

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
	dtparserparse.c dtparserlex.c

.PATH:	${TOPDIR}
SRCS+=  aslcompilerparse.c aslcompilerlex.c adfile.c \
	aslanalyze.c aslbtypes.c aslcodegen.c aslcompile.c      \
	aslerror.c aslfiles.c aslfold.c asllength.c             \
	asllisting.c aslload.c asllookup.c aslmain.c aslmap.c   \
	aslopcodes.c asloperands.c aslopt.c aslpredef.c         \
	aslresource.c aslrestype1.c aslrestype1i.c              \
	aslrestype2.c aslrestype2d.c aslrestype2e.c             \
	aslrestype2q.c aslrestype2w.c aslstartup.c aslstubs.c   \
	asltransform.c asltree.c aslutils.c asluuid.c           \
	aslwalks.c dtcompile.c dtexpress.c dtfield.c dtio.c     \
	dtparserparse.c dtparserlex.c				\
	dtsubtable.c dttable.c dttemplate.c dtutils.c

.PATH: ${TOPDIR}/../common
SRCS+=  adisasm.c adwalk.c dmextern.c dmrestag.c dmtable.c \
	dmtbdump.c dmtbinfo.c getopt.c

.PATH: ${TOPDIR}/../debugger
SRCS+=  dbfileio.c

.PATH: ${TOPDIR}/../disassembler
SRCS+=  dmbuffer.c dmnames.c dmobject.c dmopcode.c dmresrc.c    \
	dmresrcl.c dmresrcs.c dmutils.c dmwalk.c

.PATH: ${TOPDIR}/../dispatcher
SRCS+=  dsargs.c dscontrol.c dsfield.c dsobject.c dsopcode.c    \
	dsutils.c dswexec.c dswload.c dswload2.c dswscope.c     \
	dswstate.c

.PATH: ${TOPDIR}/../executer
SRCS+=  exconvrt.c excreate.c exdump.c exmisc.c exmutex.c       \
	exnames.c exoparg1.c exoparg2.c exoparg3.c exoparg6.c   \
	exprep.c exregion.c exresnte.c exresolv.c exresop.c     \
	exstore.c exstoren.c exstorob.c exsystem.c exutils.c

.PATH: ${TOPDIR}/../parser
SRCS+=  psargs.c psloop.c psopcode.c psparse.c psscope.c        \
	pstree.c psutils.c pswalk.c

.PATH: ${TOPDIR}/../tables
SRCS+=	tbfadt.c tbinstal.c tbutils.c tbxface.c

.PATH: ${TOPDIR}/../namespace
SRCS+=  nsaccess.c nsalloc.c nsdump.c nsnames.c nsobject.c      \
	nsparse.c nssearch.c nsutils.c nswalk.c nsxfobj.c

.PATH: ${TOPDIR}/../utilities
SRCS+=  utalloc.c utcache.c utcopy.c utdebug.c utdecode.c       \
	utdelete.c utglobal.c utinit.c utlock.c utmath.c        \
	utmisc.c utmutex.c utobject.c utosi.c utresrc.c         \
	utstate.c utxface.c utxferror.c

.PATH: ${TOPDIR}/../os_specific/service_layers
SRCS+=	osunixxf.c

aslcompilerparse.c: ${TOPDIR}/aslcompiler.y
	${YACC} -v -d -o${.TARGET} -pAslCompiler ${.ALLSRC}
	mv -f ${.PREFIX}.h aslcompiler.y.h

aslcompilerlex.c: ${TOPDIR}/aslcompiler.l
	${LEX} -i -PAslCompiler -o${.TARGET} ${.ALLSRC}

dtparserparse.c: ${TOPDIR}/dtparser.y
	${YACC} -v -d -o${.TARGET} -pDtParser ${.ALLSRC}
	mv -f ${.PREFIX}.h dtparser.y.h

dtparserlex.c: ${TOPDIR}/dtparser.l
	${LEX} -i -PDtParser -o${.TARGET} ${.ALLSRC}

CLEANFILES+=	aslcompilerlex.c aslcompilerparse.c	\
		aslcompiler.y.h aslcompilerparse.output \
		dtparserlex.c dtparserparse.c		\
		dtparser.y.h dtparserparse.output

.include <bsd.prog.mk>
