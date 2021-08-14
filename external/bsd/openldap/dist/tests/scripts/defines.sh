#! /bin/sh
# $OpenLDAP$
## This work is part of OpenLDAP Software <http://www.openldap.org/>.
##
## Copyright 1998-2021 The OpenLDAP Foundation.
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted only as authorized by the OpenLDAP
## Public License.
##
## A copy of this license is available in the file LICENSE in the
## top-level directory of the distribution or, alternatively, at
## <http://www.OpenLDAP.org/license.html>.

umask 077

TESTWD=`pwd`

# backends
BACKLDAP=${AC_ldap-ldapno}
BACKMETA=${AC_meta-metano}
BACKASYNCMETA=${AC_asyncmeta-asyncmetano}
BACKPERL=${AC_perl-perlno}
BACKRELAY=${AC_relay-relayno}
BACKSQL=${AC_sql-sqlno}
	RDBMS=${SLAPD_USE_SQL-rdbmsno}
	RDBMSWRITE=${SLAPD_USE_SQLWRITE-no}

# overlays
ACCESSLOG=${AC_accesslog-accesslogno}
ARGON2=${AC_argon2-argon2no}
AUTOCA=${AC_autoca-autocano}
CONSTRAINT=${AC_constraint-constraintno}
DDS=${AC_dds-ddsno}
DEREF=${AC_deref-derefno}
DYNLIST=${AC_dynlist-dynlistno}
HOMEDIR=${AC_homedir-homedirno}
MEMBEROF=${AC_memberof-memberofno}
OTP=${AC_otp-otpno}
PROXYCACHE=${AC_pcache-pcacheno}
PPOLICY=${AC_ppolicy-ppolicyno}
REFINT=${AC_refint-refintno}
REMOTEAUTH=${AC_remoteauth-remoteauthno}
RETCODE=${AC_retcode-retcodeno}
RWM=${AC_rwm-rwmno}
SYNCPROV=${AC_syncprov-syncprovno}
TRANSLUCENT=${AC_translucent-translucentno}
UNIQUE=${AC_unique-uniqueno}
VALSORT=${AC_valsort-valsortno}

# misc
WITH_SASL=${AC_WITH_SASL-no}
USE_SASL=${SLAPD_USE_SASL-no}
WITH_TLS=${AC_WITH_TLS-no}
WITH_TLS_TYPE=${AC_TLS_TYPE-no}

ACI=${AC_ACI_ENABLED-acino}
SLEEP0=${SLEEP0-1}
SLEEP1=${SLEEP1-7}
SLEEP2=${SLEEP2-15}
TIMEOUT=${TIMEOUT-8}

# dirs
PROGDIR=./progs
DATADIR=${USER_DATADIR-./testdata}
TESTDIR=${USER_TESTDIR-$TESTWD/testrun}
SCHEMADIR=${USER_SCHEMADIR-./schema}
case "$SCHEMADIR" in
.*)	ABS_SCHEMADIR="$TESTWD/$SCHEMADIR" ;;
*)  ABS_SCHEMADIR="$SCHEMADIR" ;;
esac
case "$SRCDIR" in
.*)	ABS_SRCDIR="$TESTWD/$SRCDIR" ;;
*)  ABS_SRCDIR="$SRCDIR" ;;
esac
export TESTDIR

DBDIR1A=$TESTDIR/db.1.a
DBDIR1B=$TESTDIR/db.1.b
DBDIR1C=$TESTDIR/db.1.c
DBDIR1D=$TESTDIR/db.1.d
DBDIR1=$DBDIR1A
DBDIR2A=$TESTDIR/db.2.a
DBDIR2B=$TESTDIR/db.2.b
DBDIR2C=$TESTDIR/db.2.c
DBDIR2=$DBDIR2A
DBDIR3=$TESTDIR/db.3.a
DBDIR4=$TESTDIR/db.4.a
DBDIR5=$TESTDIR/db.5.a
DBDIR6=$TESTDIR/db.6.a
SQLCONCURRENCYDIR=$DATADIR/sql-concurrency

CLIENTDIR=../clients/tools
#CLIENTDIR=/usr/local/bin

# conf
CONF=$DATADIR/slapd.conf
CONFTWO=$DATADIR/slapd2.conf
CONF2DB=$DATADIR/slapd-2db.conf
MCONF=$DATADIR/slapd-provider.conf
COMPCONF=$DATADIR/slapd-component.conf
PWCONF=$DATADIR/slapd-pw.conf
WHOAMICONF=$DATADIR/slapd-whoami.conf
ACLCONF=$DATADIR/slapd-acl.conf
RCONF=$DATADIR/slapd-referrals.conf
SRPROVIDERCONF=$DATADIR/slapd-syncrepl-provider.conf
DSRPROVIDERCONF=$DATADIR/slapd-deltasync-provider.conf
DSRCONSUMERCONF=$DATADIR/slapd-deltasync-consumer.conf
PPOLICYCONF=$DATADIR/slapd-ppolicy.conf
PROXYCACHECONF=$DATADIR/slapd-proxycache.conf
PROXYAUTHZCONF=$DATADIR/slapd-proxyauthz.conf
CACHEPROVIDERCONF=$DATADIR/slapd-cache-provider.conf
PROXYAUTHZPROVIDERCONF=$DATADIR/slapd-cache-provider-proxyauthz.conf
R1SRCONSUMERCONF=$DATADIR/slapd-syncrepl-consumer-refresh1.conf
R2SRCONSUMERCONF=$DATADIR/slapd-syncrepl-consumer-refresh2.conf
P1SRCONSUMERCONF=$DATADIR/slapd-syncrepl-consumer-persist1.conf
P2SRCONSUMERCONF=$DATADIR/slapd-syncrepl-consumer-persist2.conf
P3SRCONSUMERCONF=$DATADIR/slapd-syncrepl-consumer-persist3.conf
DIRSYNC1CONF=$DATADIR/slapd-dirsync1.conf
DSEESYNC1CONF=$DATADIR/slapd-dsee-consumer1.conf
DSEESYNC2CONF=$DATADIR/slapd-dsee-consumer2.conf
REFCONSUMERCONF=$DATADIR/slapd-ref-consumer.conf
SCHEMACONF=$DATADIR/slapd-schema.conf
TLSCONF=$DATADIR/slapd-tls.conf
TLSSASLCONF=$DATADIR/slapd-tls-sasl.conf
GLUECONF=$DATADIR/slapd-glue.conf
REFINTCONF=$DATADIR/slapd-refint.conf
RETCODECONF=$DATADIR/slapd-retcode.conf
UNIQUECONF=$DATADIR/slapd-unique.conf
LIMITSCONF=$DATADIR/slapd-limits.conf
DNCONF=$DATADIR/slapd-dn.conf
EMPTYDNCONF=$DATADIR/slapd-emptydn.conf
IDASSERTCONF=$DATADIR/slapd-idassert.conf
LDAPGLUECONF1=$DATADIR/slapd-ldapglue.conf
LDAPGLUECONF2=$DATADIR/slapd-ldapgluepeople.conf
LDAPGLUECONF3=$DATADIR/slapd-ldapgluegroups.conf
RELAYCONF=$DATADIR/slapd-relay.conf
CHAINCONF1=$DATADIR/slapd-chain1.conf
CHAINCONF2=$DATADIR/slapd-chain2.conf
GLUESYNCCONF1=$DATADIR/slapd-glue-syncrepl1.conf
GLUESYNCCONF2=$DATADIR/slapd-glue-syncrepl2.conf
SQLCONF=$DATADIR/slapd-sql.conf
SQLSRPROVIDERCONF=$DATADIR/slapd-sql-syncrepl-provider.conf
TRANSLUCENTLOCALCONF=$DATADIR/slapd-translucent-local.conf
TRANSLUCENTREMOTECONF=$DATADIR/slapd-translucent-remote.conf
METACONF=$DATADIR/slapd-meta.conf
METACONF1=$DATADIR/slapd-meta-target1.conf
METACONF2=$DATADIR/slapd-meta-target2.conf
ASYNCMETACONF=$DATADIR/slapd-asyncmeta.conf
GLUELDAPCONF=$DATADIR/slapd-glue-ldap.conf
ACICONF=$DATADIR/slapd-aci.conf
VALSORTCONF=$DATADIR/slapd-valsort.conf
DEREFCONF=$DATADIR/slapd-deref.conf
DYNLISTCONF=$DATADIR/slapd-dynlist.conf
HOMEDIRCONF=$DATADIR/slapd-homedir.conf
RCONSUMERCONF=$DATADIR/slapd-repl-consumer-remote.conf
PLSRCONSUMERCONF=$DATADIR/slapd-syncrepl-consumer-persist-ldap.conf
PLSRPROVIDERCONF=$DATADIR/slapd-syncrepl-multiproxy.conf
DDSCONF=$DATADIR/slapd-dds.conf
PASSWDCONF=$DATADIR/slapd-passwd.conf
UNDOCONF=$DATADIR/slapd-config-undo.conf
NAKEDCONF=$DATADIR/slapd-config-naked.conf
VALREGEXCONF=$DATADIR/slapd-valregex.conf

DYNAMICCONF=$DATADIR/slapd-dynamic.ldif

SLAPDLLOADCONF=$DATADIR/slapd-lload.conf
LLOADDCONF=$DATADIR/lloadd.conf
LLOADDEMPTYCONF=$DATADIR/lloadd-empty.conf
LLOADDANONCONF=$DATADIR/lloadd-anon.conf
LLOADDUNREACHABLECONF=$DATADIR/lloadd-backend-issues.conf
LLOADDTLSCONF=$DATADIR/lloadd-tls.conf
LLOADDSASLCONF=$DATADIR/lloadd-sasl.conf

# generated files
CONF1=$TESTDIR/slapd.1.conf
CONF2=$TESTDIR/slapd.2.conf
CONF3=$TESTDIR/slapd.3.conf
CONF4=$TESTDIR/slapd.4.conf
CONF5=$TESTDIR/slapd.5.conf
CONF6=$TESTDIR/slapd.6.conf
ADDCONF=$TESTDIR/slapadd.conf
CONFLDIF=$TESTDIR/slapd-dynamic.ldif

LOG1=$TESTDIR/slapd.1.log
LOG2=$TESTDIR/slapd.2.log
LOG3=$TESTDIR/slapd.3.log
LOG4=$TESTDIR/slapd.4.log
LOG5=$TESTDIR/slapd.5.log
LOG6=$TESTDIR/slapd.6.log
SLAPADDLOG1=$TESTDIR/slapadd.1.log
SLURPLOG=$TESTDIR/slurp.log

CONFIGPWF=$TESTDIR/configpw

LIBTOOL="${LIBTOOL-$TESTWD/../libtool}"
# wrappers (valgrind, gdb, environment variables, etc.)
if [ -n "$WRAPPER" ]; then
	: # skip
elif [ "$SLAPD_COMMON_WRAPPER" = gdb ]; then
	WRAPPER="$ABS_SRCDIR/scripts/grandchild_wrapper.py gdb -nx -x $ABS_SRCDIR/scripts/gdb.py -batch-silent -return-child-result --args"
elif [ "$SLAPD_COMMON_WRAPPER" = valgrind ]; then
	WRAPPER="valgrind --log-file=$TESTDIR/valgrind.%p.log --fullpath-after=`dirname $ABS_SRCDIR` --keep-debuginfo=yes --leak-check=full"
elif [ "$SLAPD_COMMON_WRAPPER" = "valgrind-errstop" ]; then
	WRAPPER="valgrind --log-file=$TESTDIR/valgrind.%p.log --vgdb=yes --vgdb-error=1"
elif [ "$SLAPD_COMMON_WRAPPER" = vgdb ]; then
	WRAPPER="valgrind --log-file=$TESTDIR/valgrind.%p.log --vgdb=yes --vgdb-error=0"
fi

if [ -n "$WRAPPER" ]; then
	SLAPD_WRAPPER="$LIBTOOL --mode=execute env $WRAPPER"
fi

# args
SASLARGS="-Q"
TOOLARGS="-x $LDAP_TOOLARGS"
TOOLPROTO="-P 3"

# cmds
CONFFILTER=$SRCDIR/scripts/conf.sh
CONFDIRSYNC=$SRCDIR/scripts/confdirsync.sh

MONITORDATA=$SRCDIR/scripts/monitor_data.sh

SLAPADD="$SLAPD_WRAPPER $TESTWD/../servers/slapd/slapd -Ta -d 0 $LDAP_VERBOSE"
SLAPCAT="$SLAPD_WRAPPER $TESTWD/../servers/slapd/slapd -Tc -d 0 $LDAP_VERBOSE"
SLAPINDEX="$SLAPD_WRAPPER $TESTWD/../servers/slapd/slapd -Ti -d 0 $LDAP_VERBOSE"
SLAPMODIFY="$SLAPD_WRAPPER $TESTWD/../servers/slapd/slapd -Tm -d 0 $LDAP_VERBOSE"
SLAPPASSWD="$SLAPD_WRAPPER $TESTWD/../servers/slapd/slapd -Tpasswd"

unset DIFF_OPTIONS
# NOTE: -u/-c is not that portable...
DIFF="diff -i"
CMP="diff -i"
BCMP="diff -iB"
CMPOUT=/dev/null
SLAPD="$SLAPD_WRAPPER $TESTWD/../servers/slapd/slapd -s0"
LLOADD="$SLAPD_WRAPPER $TESTWD/../servers/lloadd/lloadd -s0"
LDAPPASSWD="$CLIENTDIR/ldappasswd $TOOLARGS"
LDAPSASLSEARCH="$CLIENTDIR/ldapsearch $SASLARGS $TOOLPROTO $LDAP_TOOLARGS -LLL"
LDAPSASLWHOAMI="$CLIENTDIR/ldapwhoami $SASLARGS $LDAP_TOOLARGS"
LDAPSEARCH="$CLIENTDIR/ldapsearch $TOOLPROTO $TOOLARGS -LLL"
LDAPRSEARCH="$CLIENTDIR/ldapsearch $TOOLPROTO $TOOLARGS"
LDAPDELETE="$CLIENTDIR/ldapdelete $TOOLPROTO $TOOLARGS"
LDAPMODIFY="$CLIENTDIR/ldapmodify $TOOLPROTO $TOOLARGS"
LDAPADD="$CLIENTDIR/ldapmodify -a $TOOLPROTO $TOOLARGS"
LDAPMODRDN="$CLIENTDIR/ldapmodrdn $TOOLPROTO $TOOLARGS"
LDAPWHOAMI="$CLIENTDIR/ldapwhoami $TOOLARGS"
LDAPCOMPARE="$CLIENTDIR/ldapcompare $TOOLARGS"
LDAPEXOP="$CLIENTDIR/ldapexop $TOOLARGS"
SLAPDTESTER=$PROGDIR/slapd-tester
LDIFFILTER=$PROGDIR/ldif-filter
SLAPDMTREAD=$PROGDIR/slapd-mtread
LVL=${SLAPD_DEBUG-0x4105}
LOCALHOST=localhost
LOCALIP=127.0.0.1
BASEPORT=${SLAPD_BASEPORT-9010}
PORT1=`expr $BASEPORT + 1`
PORT2=`expr $BASEPORT + 2`
PORT3=`expr $BASEPORT + 3`
PORT4=`expr $BASEPORT + 4`
PORT5=`expr $BASEPORT + 5`
PORT6=`expr $BASEPORT + 6`
KDCPORT=`expr $BASEPORT + 7`
URI1="ldap://${LOCALHOST}:$PORT1/"
URIP1="ldap://${LOCALIP}:$PORT1/"
URI2="ldap://${LOCALHOST}:$PORT2/"
URIP2="ldap://${LOCALIP}:$PORT2/"
URI3="ldap://${LOCALHOST}:$PORT3/"
URIP3="ldap://${LOCALIP}:$PORT3/"
URI4="ldap://${LOCALHOST}:$PORT4/"
URIP4="ldap://${LOCALIP}:$PORT4/"
URI5="ldap://${LOCALHOST}:$PORT5/"
URIP5="ldap://${LOCALIP}:$PORT5/"
URI6="ldap://${LOCALHOST}:$PORT6/"
URIP6="ldap://${LOCALIP}:$PORT6/"
SURI1="ldaps://${LOCALHOST}:$PORT1/"
SURIP1="ldaps://${LOCALIP}:$PORT1/"
SURI2="ldaps://${LOCALHOST}:$PORT2/"
SURIP2="ldaps://${LOCALIP}:$PORT2/"
SURI3="ldaps://${LOCALHOST}:$PORT3/"
SURIP3="ldaps://${LOCALIP}:$PORT3/"
SURI4="ldaps://${LOCALHOST}:$PORT4/"
SURIP4="ldaps://${LOCALIP}:$PORT4/"
SURI5="ldaps://${LOCALHOST}:$PORT5/"
SURIP5="ldaps://${LOCALIP}:$PORT5/"
SURI6="ldaps://${LOCALHOST}:$PORT6/"
SURIP6="ldaps://${LOCALIP}:$PORT6/"

KRB5REALM="K5.REALM"
KDCHOST=$LOCALHOST

# LDIF
LDIF=$DATADIR/test.ldif
LDIFADD1=$DATADIR/do_add.1
LDIFGLUED=$DATADIR/test-glued.ldif
LDIFORDERED=$DATADIR/test-ordered.ldif
LDIFORDEREDCP=$DATADIR/test-ordered-cp.ldif
LDIFORDEREDNOCP=$DATADIR/test-ordered-nocp.ldif
LDIFBASE=$DATADIR/test-base.ldif
LDIFPASSWD=$DATADIR/passwd.ldif
LDIFWHOAMI=$DATADIR/test-whoami.ldif
LDIFPASSWDOUT=$DATADIR/passwd-out.ldif
LDIFPPOLICY=$DATADIR/ppolicy.ldif
LDIFLANG=$DATADIR/test-lang.ldif
LDIFLANGOUT=$DATADIR/lang-out.ldif
LDIFREF=$DATADIR/referrals.ldif
LDIFREFINT=$DATADIR/test-refint.ldif
LDIFUNIQUE=$DATADIR/test-unique.ldif
LDIFLIMITS=$DATADIR/test-limits.ldif
LDIFDN=$DATADIR/test-dn.ldif
LDIFEMPTYDN1=$DATADIR/test-emptydn1.ldif
LDIFEMPTYDN2=$DATADIR/test-emptydn2.ldif
LDIFIDASSERT1=$DATADIR/test-idassert1.ldif
LDIFIDASSERT2=$DATADIR/test-idassert2.ldif
LDIFLDAPGLUE1=$DATADIR/test-ldapglue.ldif
LDIFLDAPGLUE2=$DATADIR/test-ldapgluepeople.ldif
LDIFLDAPGLUE3=$DATADIR/test-ldapgluegroups.ldif
LDIFCOMPMATCH=$DATADIR/test-compmatch.ldif
LDIFCHAIN1=$DATADIR/test-chain1.ldif
LDIFCHAIN2=$DATADIR/test-chain2.ldif
LDIFTRANSLUCENTDATA=$DATADIR/test-translucent-data.ldif
LDIFTRANSLUCENTCONFIG=$DATADIR/test-translucent-config.ldif
LDIFTRANSLUCENTADD=$DATADIR/test-translucent-add.ldif
LDIFTRANSLUCENTMERGED=$DATADIR/test-translucent-merged.ldif
LDIFMETA=$DATADIR/test-meta.ldif
LDIFDEREF=$DATADIR/test-deref.ldif
LDIFVALSORT=$DATADIR/test-valsort.ldif
SQLADD=$DATADIR/sql-add.ldif
LDIFUNORDERED=$DATADIR/test-unordered.ldif
LDIFREORDERED=$DATADIR/test-reordered.ldif
LDIFMODIFY=$DATADIR/test-modify.ldif
LDIFDIRSYNCCP=$DATADIR/test-dirsync-cp.ldif
LDIFDIRSYNCNOCP=$DATADIR/test-dirsync-nocp.ldif

# strings
MONITOR=""
REFDN="c=US"
BASEDN="dc=example,dc=com"
MANAGERDN="cn=Manager,$BASEDN"
UPDATEDN="cn=consumer,$BASEDN"
PASSWD=secret
BABSDN="cn=Barbara Jensen,ou=Information Technology DivisioN,ou=People,$BASEDN"
BJORNSDN="cn=Bjorn Jensen,ou=Information Technology DivisioN,ou=People,$BASEDN"
BADBJORNSDN="cn=Bjorn JensenNotReally,ou=Information Technology DivisioN,ou=People,$BASEDN"
JAJDN="cn=James A Jones 1,ou=Alumni Association,ou=People,$BASEDN"
JOHNDDN="cn=John Doe,ou=Information Technology Division,ou=People,$BASEDN"
MELLIOTDN="cn=Mark Elliot,ou=Alumni Association,ou=People,$BASEDN"
REFINTDN="cn=Manager,o=refint"
RETCODEDN="ou=RetCodes,$BASEDN"
UNIQUEDN="cn=Manager,o=unique"
EMPTYDNDN="cn=Manager,c=US"
TRANSLUCENTROOT="o=translucent"
TRANSLUCENTUSER="ou=users,o=translucent"
TRANSLUCENTDN="uid=binder,o=translucent"
TRANSLUCENTPASSWD="bindtest"
METABASEDN="ou=Meta,$BASEDN"
METAMANAGERDN="cn=Manager,$METABASEDN"
DEREFDN="cn=Manager,o=deref"
DEREFBASEDN="o=deref"
VALSORTDN="cn=Manager,o=valsort"
VALSORTBASEDN="o=valsort"
MONITORDN="cn=Monitor"
OPERATIONSMONITORDN="cn=Operations,$MONITORDN"
CONNECTIONSMONITORDN="cn=Connections,$MONITORDN"
DATABASESMONITORDN="cn=Databases,$MONITORDN"
STATISTICSMONITORDN="cn=Statistics,$MONITORDN"

# generated outputs
SEARCHOUT=$TESTDIR/ldapsearch.out
SEARCHOUT2=$TESTDIR/ldapsearch2.out
SEARCHFLT=$TESTDIR/ldapsearch.flt
SEARCHFLT2=$TESTDIR/ldapsearch2.flt
LDIFFLT=$TESTDIR/ldif.flt
LDIFFLT2=$TESTDIR/ldif2.flt
TESTOUT=$TESTDIR/test.out
INITOUT=$TESTDIR/init.out
VALSORTOUT1=$DATADIR/valsort1.out
VALSORTOUT2=$DATADIR/valsort2.out
VALSORTOUT3=$DATADIR/valsort3.out
MONITOROUT1=$DATADIR/monitor1.out
MONITOROUT2=$DATADIR/monitor2.out
MONITOROUT3=$DATADIR/monitor3.out
MONITOROUT4=$DATADIR/monitor4.out

SERVER1OUT=$TESTDIR/server1.out
SERVER1FLT=$TESTDIR/server1.flt
SERVER2OUT=$TESTDIR/server2.out
SERVER2FLT=$TESTDIR/server2.flt
SERVER3OUT=$TESTDIR/server3.out
SERVER3FLT=$TESTDIR/server3.flt
SERVER4OUT=$TESTDIR/server4.out
SERVER4FLT=$TESTDIR/server4.flt
SERVER5OUT=$TESTDIR/server5.out
SERVER5FLT=$TESTDIR/server5.flt
SERVER6OUT=$TESTDIR/server6.out
SERVER6FLT=$TESTDIR/server6.flt

PROVIDEROUT=$SERVER1OUT
PROVIDERFLT=$SERVER1FLT
CONSUMEROUT=$SERVER2OUT
CONSUMER2OUT=$SERVER3OUT
CONSUMERFLT=$SERVER2FLT
CONSUMER2FLT=$SERVER3FLT

MTREADOUT=$TESTDIR/mtread.out

# original outputs for cmp
PROXYCACHEOUT=$DATADIR/proxycache.out
REFERRALOUT=$DATADIR/referrals.out
SEARCHOUTPROVIDER=$DATADIR/search.out.provider
SEARCHOUTX=$DATADIR/search.out.xsearch
COMPSEARCHOUT=$DATADIR/compsearch.out
MODIFYOUTPROVIDER=$DATADIR/modify.out.provider
ADDDELOUTPROVIDER=$DATADIR/adddel.out.provider
MODRDNOUTPROVIDER0=$DATADIR/modrdn.out.provider.0
MODRDNOUTPROVIDER1=$DATADIR/modrdn.out.provider.1
MODRDNOUTPROVIDER2=$DATADIR/modrdn.out.provider.2
MODRDNOUTPROVIDER3=$DATADIR/modrdn.out.provider.3
ACLOUTPROVIDER=$DATADIR/acl.out.provider
REPLOUTPROVIDER=$DATADIR/repl.out.provider
MODSRCHFILTERS=$DATADIR/modify.search.filters
CERTIFICATETLS=$DATADIR/certificate.tls
CERTIFICATEOUT=$DATADIR/certificate.out
DNOUT=$DATADIR/dn.out
EMPTYDNOUT1=$DATADIR/emptydn.out.slapadd
EMPTYDNOUT2=$DATADIR/emptydn.out
IDASSERTOUT=$DATADIR/idassert.out
LDAPGLUEOUT=$DATADIR/ldapglue.out
LDAPGLUEANONYMOUSOUT=$DATADIR/ldapglueanonymous.out
RELAYOUT=$DATADIR/relay.out
CHAINOUT=$DATADIR/chain.out
CHAINREFOUT=$DATADIR/chainref.out
CHAINMODOUT=$DATADIR/chainmod.out
GLUESYNCOUT=$DATADIR/gluesync.out
SQLREAD=$DATADIR/sql-read.out
SQLWRITE=$DATADIR/sql-write.out
TRANSLUCENTOUT=$DATADIR/translucent.search.out
METAOUT=$DATADIR/meta.out
METACONCURRENCYOUT=$DATADIR/metaconcurrency.out
MANAGEOUT=$DATADIR/manage.out
SUBTREERENAMEOUT=$DATADIR/subtree-rename.out
ACIOUT=$DATADIR/aci.out
DYNLISTOUT=$DATADIR/dynlist.out
DDSOUT=$DATADIR/dds.out
DEREFOUT=$DATADIR/deref.out
MEMBEROFOUT=$DATADIR/memberof.out
MEMBEROFREFINTOUT=$DATADIR/memberof-refint.out
SHTOOL="$SRCDIR/../build/shtool"

