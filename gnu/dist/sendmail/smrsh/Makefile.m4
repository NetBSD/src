dnl Id: Makefile.m4,v 8.35 2002/06/21 22:01:52 ca Exp
dnl $NetBSD: Makefile.m4,v 1.1.1.6 2005/03/15 02:05:36 atatat Exp $
include(confBUILDTOOLSDIR`/M4/switch.m4')

define(`confREQUIRE_LIBSM', `true')
# sendmail dir
SMSRCDIR=	ifdef(`confSMSRCDIR', `confSMSRCDIR', `${SRCDIR}/sendmail')
PREPENDDEF(`confENVDEF', `confMAPDEF')
PREPENDDEF(`confINCDIRS', `-I${SMSRCDIR} ')

bldPRODUCT_START(`executable', `smrsh')
define(`bldINSTALL_DIR', `E')
define(`bldSOURCES', `smrsh.c ')
bldPUSH_SMLIB(`sm')
APPENDDEF(`confENVDEF', `-DNOT_SENDMAIL')
bldPRODUCT_END

bldPRODUCT_START(`manpage', `smrsh')
define(`bldSOURCES', `smrsh.8')
bldPRODUCT_END

bldFINISH
