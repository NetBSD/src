dnl Id: Makefile.m4,v 8.34.4.1 2002/06/21 21:58:37 ca Exp
dnl $NetBSD: Makefile.m4,v 1.1.1.4 2003/06/01 14:01:17 atatat Exp $
include(confBUILDTOOLSDIR`/M4/switch.m4')

define(`confREQUIRE_LIBSM', `true')
# sendmail dir
SMSRCDIR=	ifdef(`confSMSRCDIR', `confSMSRCDIR', `${SRCDIR}/sendmail')
PREPENDDEF(`confENVDEF', `confMAPDEF')
PREPENDDEF(`confINCDIRS', `-I${SMSRCDIR} ')

bldPRODUCT_START(`executable', `mailstats')
define(`bldINSTALL_DIR', `S')
define(`bldSOURCES', `mailstats.c ')
bldPUSH_SMLIB(`sm')
bldPUSH_SMLIB(`smutil')
APPENDDEF(`confENVDEF', `-DNOT_SENDMAIL')
bldPRODUCT_END

bldPRODUCT_START(`manpage', `mailstats')
define(`bldSOURCES', `mailstats.8')
bldPRODUCT_END

bldFINISH

