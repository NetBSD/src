dnl Id: Makefile.m4,v 8.14 2002/06/21 22:01:34 ca Exp
dnl $NetBSD: Makefile.m4,v 1.1.1.4 2005/03/15 02:06:05 atatat Exp $
include(confBUILDTOOLSDIR`/M4/switch.m4')

define(`confREQUIRE_LIBSM', `true')
# sendmail dir
SMSRCDIR=	ifdef(`confSMSRCDIR', `confSMSRCDIR', `${SRCDIR}/sendmail')
PREPENDDEF(`confENVDEF', `confMAPDEF')
PREPENDDEF(`confINCDIRS', `-I${SMSRCDIR} ')

bldPRODUCT_START(`library', `libsmdb')
define(`bldSOURCES', `smdb.c smdb1.c smdb2.c smndbm.c ')
APPENDDEF(`confENVDEF', `-DNOT_SENDMAIL')
bldPRODUCT_END

bldFINISH
