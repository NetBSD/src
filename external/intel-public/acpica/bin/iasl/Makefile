#	$NetBSD: Makefile,v 1.1 2009/08/18 23:42:40 jmcneill Exp $

PROG=	iasl
BINDIR=	/usr/bin
NOMAN=	1
WARNS=	0
TOPDIR=	${NETBSDSRCDIR}/sys/external/intel-public/acpica/dist/compiler

.include <bsd.own.mk>

CPPFLAGS+=	-I${TOPDIR}/../include -I${TOPDIR} -I.
CPPFLAGS+=	-D_LINUX -DACPI_ASL_COMPILER -DACPI_USE_ALTERNATE_TIMEOUT
CPPFLAGS+=	-D_USE_BERKELEY_YACC
LDADD+=		-ll -ly -lrt -lpthread
DPADD+=		${LIBL} ${LIBY} ${LIBRT} ${LIBPTHREAD}

DPSRCS=	aslcompilerparse.c
DPSRCS+=aslcompilerlex.c

.PATH:	${TOPDIR}
SRCS=	aslcompilerparse.c
SRCS+=	aslcompilerlex.c
SRCS+=	aslanalyze.c
SRCS+=	aslcodegen.c
SRCS+=	aslcompile.c
SRCS+=	aslerror.c
SRCS+=	aslfiles.c
SRCS+=	asllength.c
SRCS+=	asllisting.c
SRCS+=	aslload.c
SRCS+=	asllookup.c
SRCS+=	aslmain.c
SRCS+=	aslmap.c
SRCS+=	aslopcodes.c
SRCS+=	asloperands.c
SRCS+=	aslresource.c
SRCS+=	aslrestype1.c
SRCS+=	aslrestype2.c
SRCS+=	aslstartup.c
SRCS+=	asltree.c
SRCS+=	aslutils.c
SRCS+=	asltransform.c
SRCS+=	aslfold.c
SRCS+=	aslstubs.c
SRCS+=	aslopt.c
.PATH: ${TOPDIR}/../common
SRCS+=	getopt.c
.PATH: ${TOPDIR}/../utilities
SRCS+=	utalloc.c
SRCS+=	utcache.c
SRCS+=	utcopy.c
SRCS+=	utdebug.c
SRCS+=	utdelete.c
SRCS+=	utglobal.c
SRCS+=	utinit.c
SRCS+=	utlock.c
SRCS+=	utobject.c
SRCS+=	utmisc.c
SRCS+=	utmath.c
SRCS+=	utmutex.c
SRCS+=	utresrc.c
SRCS+=	utstate.c
SRCS+=	utxface.c
.PATH: ${TOPDIR}/../namespace
SRCS+=	nsaccess.c
SRCS+=	nsalloc.c
SRCS+=	nsdump.c
SRCS+=	nsnames.c
SRCS+=	nsobject.c
SRCS+=	nsparse.c
SRCS+=	nssearch.c
SRCS+=	nsutils.c
SRCS+=	nswalk.c
SRCS+=	nsxfobj.c
.PATH: ${TOPDIR}/../parser
SRCS+=	psargs.c
SRCS+=	psloop.c
SRCS+=	psopcode.c
SRCS+=	psparse.c
SRCS+=	psscope.c
SRCS+=	pstree.c
SRCS+=	psutils.c
SRCS+=	pswalk.c
.PATH: ${TOPDIR}/../dispatcher
SRCS+=	dswscope.c
SRCS+=	dswstate.c
SRCS+=	dsfield.c
SRCS+=	dsobject.c
SRCS+=	dsopcode.c
SRCS+=	dsutils.c
SRCS+=	dswexec.c
SRCS+=	dswload.c
.PATH: ${TOPDIR}/../executer
SRCS+=	exconvrt.c
SRCS+=	excreate.c
SRCS+=	exdump.c
SRCS+=	exmisc.c
SRCS+=	exmutex.c
SRCS+=	exnames.c
SRCS+=	exoparg1.c
SRCS+=	exoparg2.c
SRCS+=	exoparg3.c
SRCS+=	exoparg6.c
SRCS+=	exprep.c
SRCS+=	exregion.c
SRCS+=	exresnte.c
SRCS+=	exresolv.c
SRCS+=	exresop.c
SRCS+=	exstore.c
SRCS+=	exstoren.c
SRCS+=	exstorob.c
SRCS+=	exsystem.c
SRCS+=	exutils.c
.PATH: ${TOPDIR}/../common
SRCS+=	adfile.c
SRCS+=	adisasm.c
SRCS+=	adwalk.c
SRCS+=	dmrestag.c
SRCS+=	dmtable.c
SRCS+=	dmtbinfo.c
SRCS+=	dmtbdump.c
.PATH: ${TOPDIR}/../debugger
SRCS+=	dbfileio.c
.PATH: ${TOPDIR}/../disassembler
SRCS+=	dmbuffer.c
SRCS+=	dmnames.c
SRCS+=	dmopcode.c
SRCS+=	dmobject.c
SRCS+=	dmresrc.c
SRCS+=	dmresrcl.c
SRCS+=	dmresrcs.c
SRCS+=	dmutils.c
SRCS+=	dmwalk.c
.PATH: ${TOPDIR}/../tables
SRCS+=	tbfadt.c
SRCS+=	tbinstal.c
SRCS+=	tbutils.c
SRCS+=	tbxface.c
.PATH: ${TOPDIR}/..
SRCS+=	osunixxf.c

aslcompilerparse.c: ${TOPDIR}/aslcompiler.y
	${YACC} -v -d -o${.TARGET} -pAslCompiler ${.ALLSRC}
	mv -f ${.PREFIX}.h aslcompiler.y.h

aslcompilerlex.c: ${TOPDIR}/aslcompiler.l
	${LEX} -i -PAslCompiler -o${.TARGET} ${.ALLSRC}

CLEANFILES+=	aslcompilerlex.c aslcompilerparse.c aslcompiler.y.h y.output

.include <bsd.prog.mk>
