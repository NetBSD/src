#!/bin/sh

# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0.  If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

DIGOPTS="-p ${PORT}"
RNDCCMD="$RNDC -c $SYSTEMTESTTOP/common/rndc.conf -p ${CONTROLPORT} -s"
SEND="$PERL $SYSTEMTESTTOP/send.pl 10.53.0.4 ${EXTRAPORT1}"
status=0
n=0

n=`expr $n + 1`
echo_i "checking short DNAME from authoritative ($n)"
ret=0
$DIG $DIGOPTS a.short-dname.example @10.53.0.2 a > dig.out.ns2.short || ret=1
grep "status: NOERROR" dig.out.ns2.short > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking short DNAME from recursive ($n)"
ret=0
$RNDCCMD 10.53.0.7 null --- start test$n --- 2>&1 | sed 's/^/ns7 /' | cat_i
$DIG $DIGOPTS a.short-dname.example @10.53.0.7 a > dig.out.ns4.short || ret=1
grep "status: NOERROR" dig.out.ns4.short > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking long DNAME from authoritative ($n)"
ret=0
$DIG $DIGOPTS a.long-dname.example @10.53.0.2 a > dig.out.ns2.long || ret=1
grep "status: NOERROR" dig.out.ns2.long > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking long DNAME from recursive ($n)"
ret=0
$RNDCCMD 10.53.0.7 null --- start test$n --- 2>&1 | sed 's/^/ns7 /' | cat_i
$DIG $DIGOPTS a.long-dname.example @10.53.0.7 a > dig.out.ns4.long || ret=1
grep "status: NOERROR" dig.out.ns4.long > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking (too) long DNAME from authoritative ($n)"
ret=0
$DIG $DIGOPTS 01234567890123456789012345678901234567890123456789.longlonglonglonglonglonglonglonglonglonglonglonglonglonglong.longlonglonglonglonglonglonglonglonglonglonglonglonglonglong.longlonglonglonglonglonglonglonglonglonglonglonglonglonglong.long-dname.example @10.53.0.2 a > dig.out.ns2.toolong || ret=1
grep "status: YXDOMAIN" dig.out.ns2.toolong > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking (too) long DNAME from recursive with cached DNAME ($n)"
ret=0
$RNDCCMD 10.53.0.7 null --- start test$n --- 2>&1 | sed 's/^/ns7 /' | cat_i
$DIG $DIGOPTS 01234567890123456789012345678901234567890123456789.longlonglonglonglonglonglonglonglonglonglonglonglonglonglong.longlonglonglonglonglonglonglonglonglonglonglonglonglonglong.longlonglonglonglonglonglonglonglonglonglonglonglonglonglong.long-dname.example @10.53.0.7 a > dig.out.ns4.cachedtoolong || ret=1
grep "status: YXDOMAIN" dig.out.ns4.cachedtoolong > /dev/null || ret=1
grep '^long-dname\.example\..*DNAME.*long' dig.out.ns4.cachedtoolong > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking (too) long DNAME from recursive without cached DNAME ($n)"
ret=0
$RNDCCMD 10.53.0.7 null --- start test$n --- 2>&1 | sed 's/^/ns7 /' | cat_i
$DIG $DIGOPTS 01234567890123456789012345678901234567890123456789.longlonglonglonglonglonglonglonglonglonglonglonglonglonglong.longlonglonglonglonglonglonglonglonglonglonglonglonglonglong.longlonglonglonglonglonglonglonglonglonglonglonglonglong.toolong-dname.example @10.53.0.7 a > dig.out.ns4.uncachedtoolong || ret=1
grep "status: YXDOMAIN" dig.out.ns4.uncachedtoolong > /dev/null || ret=1
grep '^toolong-dname\.example\..*DNAME.*long' dig.out.ns4.uncachedtoolong > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

find_records() {
	owner_name="$1"
	rr_type="$2"
	file="$3"
	awk '$1 == "'"$owner_name"'" && $4 == "'"$rr_type"'" { print }' < "$file"
}

count_records() {
	owner_name="$1"
	rr_type="$2"
	file="$3"
	find_records "$owner_name" "$rr_type" "$file" | wc -l
}

exactly_one_record_exists_for() {
	owner_name="$1"
	rr_type="$2"
	file="$3"
	test "$(count_records "$owner_name" "$rr_type" "$file")" -eq 1
}

no_records_exist_for() {
	owner_name="$1"
	rr_type="$2"
	file="$3"
	test "$(count_records "$owner_name" "$rr_type" "$file")" -eq 0
}

ensure_no_ds_in_bitmap() {
	owner_name="$1"
	rr_type="$2"
	file="$3"
	case "$rr_type" in
		NSEC) start_index=6 ;;
		NSEC3) start_index=10 ;;
		*) exit 1 ;;
	esac
	find_records "$owner_name" "$rr_type" "$file" | awk '{ for (i='"$start_index"'; i<=NF; i++) if ($i == "DS") exit 1 }'
}

n=`expr $n + 1`
echo_i "checking secure delegation prepared using CNAME chaining ($n)"
ret=0
# QNAME exists, so the AUTHORITY section should only contain an NS RRset and a
# DS RRset.
$DIG $DIGOPTS @10.53.0.2 cname.wildcard-secure.example A +norec +dnssec > dig.out.2.$n 2>&1 || ret=1
# Ensure that the AUTHORITY section contains the expected NS and DS RRsets.
exactly_one_record_exists_for "delegation.wildcard-secure.example." NS dig.out.2.$n || ret=1
exactly_one_record_exists_for "delegation.wildcard-secure.example." DS dig.out.2.$n || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking secure delegation prepared using wildcard expansion + CNAME chaining ($n)"
ret=0
# QNAME does not exist, so the AUTHORITY section should contain an NS RRset, an
# NSEC record proving nonexistence of QNAME, and a DS RRset at the zone cut.
$DIG $DIGOPTS @10.53.0.2 a-nonexistent-name.wildcard-secure.example A +norec +dnssec > dig.out.2.$n 2>&1 || ret=1
# Ensure that the AUTHORITY section contains the expected NS and DS RRsets.
exactly_one_record_exists_for "delegation.wildcard-secure.example." NS dig.out.2.$n || ret=1
exactly_one_record_exists_for "delegation.wildcard-secure.example." DS dig.out.2.$n || ret=1
# Check NSEC records in the AUTHORITY section.
no_records_exist_for "wildcard-secure.example." NSEC dig.out.2.$n || ret=1
exactly_one_record_exists_for "*.wildcard-secure.example." NSEC dig.out.2.$n || ret=1
no_records_exist_for "cname.wildcard-secure.example." NSEC dig.out.2.$n || ret=1
no_records_exist_for "delegation.wildcard-secure.example." NSEC dig.out.2.$n || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking insecure delegation prepared using CNAME chaining, NSEC ($n)"
ret=0
# QNAME exists, so the AUTHORITY section should only contain an NS RRset and a
# single NSEC record proving nonexistence of a DS RRset at the zone cut.
$DIG $DIGOPTS @10.53.0.2 cname.wildcard-nsec.example A +norec +dnssec > dig.out.2.$n 2>&1 || ret=1
# Ensure that the AUTHORITY section contains an NS RRset without an associated
# DS RRset.
exactly_one_record_exists_for "delegation.wildcard-nsec.example." NS dig.out.2.$n || ret=1
no_records_exist_for "delegation.wildcard-nsec.example." DS dig.out.2.$n || ret=1
# Check NSEC records in the AUTHORITY section.
no_records_exist_for "wildcard-nsec.example." NSEC dig.out.2.$n || ret=1
no_records_exist_for "*.wildcard-nsec.example." NSEC dig.out.2.$n || ret=1
no_records_exist_for "cname.wildcard-nsec.example." NSEC dig.out.2.$n || ret=1
exactly_one_record_exists_for "delegation.wildcard-nsec.example." NSEC dig.out.2.$n || ret=1
# Ensure the NSEC record for the zone cut does not have the DS bit set in the
# type bit map.
ensure_no_ds_in_bitmap "delegation.wildcard-nsec.example." NSEC dig.out.2.$n || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking insecure delegation prepared using wildcard expansion + CNAME chaining, NSEC, QNAME #1 ($n)"
ret=0
# QNAME does not exist, so the AUTHORITY section should contain an NS RRset and
# NSEC records proving nonexistence of both QNAME and a DS RRset at the zone
# cut.  In this test case, these two NSEC records are different.
$DIG $DIGOPTS @10.53.0.2 a-nonexistent-name.wildcard-nsec.example A +norec +dnssec > dig.out.2.$n 2>&1 || ret=1
# Ensure that the AUTHORITY section contains an NS RRset without an associated
# DS RRset.
exactly_one_record_exists_for "delegation.wildcard-nsec.example." NS dig.out.2.$n || ret=1
no_records_exist_for "delegation.wildcard-nsec.example." DS dig.out.2.$n || ret=1
# Check NSEC records in the AUTHORITY section.
no_records_exist_for "wildcard-nsec.example." NSEC dig.out.2.$n || ret=1
exactly_one_record_exists_for "*.wildcard-nsec.example." NSEC dig.out.2.$n || ret=1
no_records_exist_for "cname.wildcard-nsec.example." NSEC dig.out.2.$n || ret=1
exactly_one_record_exists_for "delegation.wildcard-nsec.example." NSEC dig.out.2.$n || ret=1
# Ensure the NSEC record for the zone cut does not have the DS bit set in the
# type bit map.
ensure_no_ds_in_bitmap "delegation.wildcard-nsec.example." NSEC dig.out.2.$n || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking insecure delegation prepared using wildcard expansion + CNAME chaining, NSEC, QNAME #2 ($n)"
ret=0
# QNAME does not exist, so the AUTHORITY section should contain an NS RRset and
# NSEC records proving nonexistence of both QNAME and a DS RRset at the zone
# cut.  In this test case, the same NSEC record proves nonexistence of both the
# QNAME and the DS RRset at the zone cut.
$DIG $DIGOPTS @10.53.0.2 z-nonexistent-name.wildcard-nsec.example A +norec +dnssec > dig.out.2.$n 2>&1 || ret=1
# Ensure that the AUTHORITY section contains an NS RRset without an associated
# DS RRset.
exactly_one_record_exists_for "delegation.wildcard-nsec.example." NS dig.out.2.$n || ret=1
no_records_exist_for "delegation.wildcard-nsec.example." DS dig.out.2.$n || ret=1
# Check NSEC records in the AUTHORITY section.
no_records_exist_for "wildcard-nsec.example." NSEC dig.out.2.$n || ret=1
no_records_exist_for "*.wildcard-nsec.example." NSEC dig.out.2.$n || ret=1
no_records_exist_for "cname.wildcard-nsec.example." NSEC dig.out.2.$n || ret=1
exactly_one_record_exists_for "delegation.wildcard-nsec.example." NSEC dig.out.2.$n || ret=1
# Ensure the NSEC record for the zone cut does not have the DS bit set in the
# type bit map.
ensure_no_ds_in_bitmap "delegation.wildcard-nsec.example." NSEC dig.out.2.$n || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Relevant NSEC3 hashes:
#
# - existing names:
#
#       $ nsec3hash - 1 0 wildcard-nsec3.example.
#       38IVP9CN0LBISO6H3V5REQCKMTHLI5AN (salt=-, hash=1, iterations=0)
#       $ nsec3hash - 1 0 cname.wildcard-nsec3.example.
#       3DV6GNNVR0O8LA4DC4CHL2JTVNHT8Q1D (salt=-, hash=1, iterations=0)
#       $ nsec3hash - 1 0 delegation.wildcard-nsec3.example.
#       AVKOGGGVJHFSLQA68TILKFKJ94AV4MNC (salt=-, hash=1, iterations=0)
#       $ nsec3hash - 1 0 *.wildcard-nsec3.example.
#       Q64D8L8HLSB3L98S59PM8OSSMI7SMQA2 (salt=-, hash=1, iterations=0)
#
# - nonexistent names:
#
#       $ nsec3hash - 1 0 a-nonexistent-name.wildcard-nsec3.example.
#       PST9IH6M0DG3M139CO3G12NUP4ER88SH (salt=-, hash=1, iterations=0)
#       $ nsec3hash - 1 0 z-nonexistent-name.wildcard-nsec3.example.
#       SG2DEHEAOGCKP7FTNQAUVC3I3TIPJH0J (salt=-, hash=1, iterations=0)

n=`expr $n + 1`
echo_i "checking insecure delegation prepared using CNAME chaining, NSEC3 ($n)"
ret=0
# QNAME exists, so the AUTHORITY section should only contain an NS RRset and a
# single NSEC3 record proving nonexistence of a DS RRset at the zone cut.
$DIG $DIGOPTS @10.53.0.2 cname.wildcard-nsec3.example A +norec +dnssec > dig.out.2.$n 2>&1 || ret=1
# Ensure that the AUTHORITY section contains an NS RRset without an associated
# DS RRset.
exactly_one_record_exists_for "delegation.wildcard-nsec3.example." NS dig.out.2.$n || ret=1
no_records_exist_for "delegation.wildcard-nsec3.example." DS dig.out.2.$n || ret=1
# Check NSEC3 records in the AUTHORITY section.
no_records_exist_for "38IVP9CN0LBISO6H3V5REQCKMTHLI5AN.wildcard-nsec3.example." NSEC3 dig.out.2.$n || ret=1
no_records_exist_for "3DV6GNNVR0O8LA4DC4CHL2JTVNHT8Q1D.wildcard-nsec3.example." NSEC3 dig.out.2.$n || ret=1
exactly_one_record_exists_for "AVKOGGGVJHFSLQA68TILKFKJ94AV4MNC.wildcard-nsec3.example." NSEC3 dig.out.2.$n || ret=1
no_records_exist_for "Q64D8L8HLSB3L98S59PM8OSSMI7SMQA2.wildcard-nsec3.example." NSEC3 dig.out.2.$n || ret=1
# Ensure the NSEC3 record matching the zone cut does not have the DS bit set in
# the type bit map.
ensure_no_ds_in_bitmap "AVKOGGGVJHFSLQA68TILKFKJ94AV4MNC.wildcard-nsec3.example." NSEC3 dig.out.2.$n || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking insecure delegation prepared using wildcard expansion + CNAME chaining, NSEC3, QNAME #1 ($n)"
ret=0
# QNAME does not exist, so the AUTHORITY section should contain an NS RRset and
# NSEC3 records proving nonexistence of both QNAME and a DS RRset at the zone
# cut.  In this test case, these two NSEC3 records are different.
$DIG $DIGOPTS @10.53.0.2 z-nonexistent-name.wildcard-nsec3.example A +norec +dnssec > dig.out.2.$n 2>&1 || ret=1
# Ensure that the AUTHORITY section contains an NS RRset without an associated
# DS RRset.
exactly_one_record_exists_for "delegation.wildcard-nsec3.example." NS dig.out.2.$n || ret=1
no_records_exist_for "delegation.wildcard-nsec3.example." DS dig.out.2.$n || ret=1
# Check NSEC3 records in the AUTHORITY section.
no_records_exist_for "38IVP9CN0LBISO6H3V5REQCKMTHLI5AN.wildcard-nsec3.example." NSEC3 dig.out.2.$n || ret=1
no_records_exist_for "3DV6GNNVR0O8LA4DC4CHL2JTVNHT8Q1D.wildcard-nsec3.example." NSEC3 dig.out.2.$n || ret=1
exactly_one_record_exists_for "AVKOGGGVJHFSLQA68TILKFKJ94AV4MNC.wildcard-nsec3.example." NSEC3 dig.out.2.$n || ret=1
exactly_one_record_exists_for "Q64D8L8HLSB3L98S59PM8OSSMI7SMQA2.wildcard-nsec3.example." NSEC3 dig.out.2.$n || ret=1
# Ensure the NSEC3 record matching the zone cut does not have the DS bit set in
# the type bit map.
ensure_no_ds_in_bitmap "AVKOGGGVJHFSLQA68TILKFKJ94AV4MNC.wildcard-nsec3.example." NSEC3 dig.out.2.$n || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking insecure delegation prepared using wildcard expansion + CNAME chaining, NSEC3, QNAME #2 ($n)"
ret=0
# QNAME does not exist, so the AUTHORITY section should contain an NS RRset and
# NSEC3 records proving nonexistence of both QNAME and a DS RRset at the zone
# cut.  In this test case, the same NSEC3 record proves nonexistence of both the
# QNAME and the DS RRset at the zone cut.
$DIG $DIGOPTS @10.53.0.2 a-nonexistent-name.wildcard-nsec3.example A +norec +dnssec > dig.out.2.$n 2>&1 || ret=1
# Ensure that the AUTHORITY section contains an NS RRset without an associated
# DS RRset.
exactly_one_record_exists_for "delegation.wildcard-nsec3.example." NS dig.out.2.$n || ret=1
no_records_exist_for "delegation.wildcard-nsec3.example." DS dig.out.2.$n || ret=1
# Check NSEC3 records in the AUTHORITY section.
no_records_exist_for "38IVP9CN0LBISO6H3V5REQCKMTHLI5AN.wildcard-nsec3.example." NSEC3 dig.out.2.$n || ret=1
no_records_exist_for "3DV6GNNVR0O8LA4DC4CHL2JTVNHT8Q1D.wildcard-nsec3.example." NSEC3 dig.out.2.$n || ret=1
exactly_one_record_exists_for "AVKOGGGVJHFSLQA68TILKFKJ94AV4MNC.wildcard-nsec3.example." NSEC3 dig.out.2.$n || ret=1
no_records_exist_for "Q64D8L8HLSB3L98S59PM8OSSMI7SMQA2.wildcard-nsec3.example." NSEC3 dig.out.2.$n || ret=1
# Ensure the NSEC3 record matching the zone cut does not have the DS bit set in
# the type bit map.
ensure_no_ds_in_bitmap "AVKOGGGVJHFSLQA68TILKFKJ94AV4MNC.wildcard-nsec3.example." NSEC3 dig.out.2.$n || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Relevant NSEC3 hashes:
#
# - existing names with corresponding NSEC3 records:
#
#       $ nsec3hash - 1 0 *.wildcard-nsec3-optout.example.
#       2JGSPT59VJ7R9SQB5B9P6HPM5JBATOOO (salt=-, hash=1, iterations=0)
#       $ nsec3hash - 1 0 cname.wildcard-nsec3-optout.example.
#       OKRFKC9SS1O60E8U2980UD62MUSMKGUG (salt=-, hash=1, iterations=0)
#       $ nsec3hash - 1 0 wildcard-nsec3-optout.example.
#       SS5M1RUBSGMANEQ1VLRDDEC6SOAT7HNI (salt=-, hash=1, iterations=0)
#
# - existing name with no corresponding NSEC3 record due to opt-out:
#
#       $ nsec3hash - 1 0 delegation.wildcard-nsec3-optout.example.
#       UFP8PVECFTD57HU5PUD2HE0ES37QEOAP (salt=-, hash=1, iterations=0)
#
# - nonexistent names:
#
#       $ nsec3hash - 1 0 b-nonexistent-name.wildcard-nsec3-optout.example.
#       3J38JE2OU0O7B4CE2ADMBBKJ5HT994S5 (salt=-, hash=1, iterations=0)
#       $ nsec3hash - 1 0 z-nonexistent-name.wildcard-nsec3-optout.example.
#       V7OTS4791T9SU0HKVL93EVNAJ9JH2CH3 (salt=-, hash=1, iterations=0)

n=`expr $n + 1`
echo_i "checking insecure delegation prepared using CNAME chaining, NSEC3 with opt-out ($n)"
ret=0
# QNAME exists, so the AUTHORITY section should only contain an NS RRset and a
# single NSEC3 record proving nonexistence of a DS RRset at the zone cut.
$DIG $DIGOPTS @10.53.0.2 cname.wildcard-nsec3-optout.example A +norec +dnssec > dig.out.2.$n 2>&1 || ret=1
# Ensure that the AUTHORITY section contains an NS RRset without an associated
# DS RRset.
exactly_one_record_exists_for "delegation.wildcard-nsec3-optout.example." NS dig.out.2.$n || ret=1
no_records_exist_for "delegation.wildcard-nsec3-optout.example." DS dig.out.2.$n || ret=1
# Check NSEC3 records in the AUTHORITY section.
no_records_exist_for "2JGSPT59VJ7R9SQB5B9P6HPM5JBATOOO.wildcard-nsec3-optout.example." NSEC3 dig.out.2.$n || ret=1
no_records_exist_for "OKRFKC9SS1O60E8U2980UD62MUSMKGUG.wildcard-nsec3-optout.example." NSEC3 dig.out.2.$n || ret=1
exactly_one_record_exists_for "SS5M1RUBSGMANEQ1VLRDDEC6SOAT7HNI.wildcard-nsec3-optout.example." NSEC3 dig.out.2.$n || ret=1
# Ensure the NSEC3 record covering the zone cut does not have the DS bit set in
# the type bit map.
ensure_no_ds_in_bitmap "SS5M1RUBSGMANEQ1VLRDDEC6SOAT7HNI.wildcard-nsec3-optout.example." NSEC3 dig.out.2.$n || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking insecure delegation prepared using wildcard expansion + CNAME chaining, NSEC3 with opt-out, QNAME #1 ($n)"
ret=0
# QNAME does not exist, so the AUTHORITY section should contain an NS RRset and
# NSEC3 records proving nonexistence of both QNAME and a DS RRset at the zone
# cut.  In this test case, these two NSEC3 records are different.
$DIG $DIGOPTS @10.53.0.2 b-nonexistent-name.wildcard-nsec3-optout.example A +norec +dnssec > dig.out.2.$n 2>&1 || ret=1
# Ensure that the AUTHORITY section contains an NS RRset without an associated
# DS RRset.
exactly_one_record_exists_for "delegation.wildcard-nsec3-optout.example." NS dig.out.2.$n || ret=1
no_records_exist_for "delegation.wildcard-nsec3-optout.example." DS dig.out.2.$n || ret=1
# Check NSEC3 records in the AUTHORITY section.
exactly_one_record_exists_for "2JGSPT59VJ7R9SQB5B9P6HPM5JBATOOO.wildcard-nsec3-optout.example." NSEC3 dig.out.2.$n || ret=1
no_records_exist_for "OKRFKC9SS1O60E8U2980UD62MUSMKGUG.wildcard-nsec3-optout.example." NSEC3 dig.out.2.$n || ret=1
exactly_one_record_exists_for "SS5M1RUBSGMANEQ1VLRDDEC6SOAT7HNI.wildcard-nsec3-optout.example." NSEC3 dig.out.2.$n || ret=1
# Ensure the NSEC3 record covering the zone cut does not have the DS bit set in
# the type bit map.
ensure_no_ds_in_bitmap "SS5M1RUBSGMANEQ1VLRDDEC6SOAT7HNI.wildcard-nsec3-optout.example." NSEC3 dig.out.2.$n || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking insecure delegation prepared using wildcard expansion + CNAME chaining, NSEC3 with opt-out, QNAME #2 ($n)"
ret=0
# QNAME does not exist, so the AUTHORITY section should contain an NS RRset and
# NSEC3 records proving nonexistence of both QNAME and a DS RRset at the zone
# cut.  In this test case, the same NSEC3 record proves nonexistence of both the
# QNAME and the DS RRset at the zone cut.
$DIG $DIGOPTS @10.53.0.2 z-nonexistent-name.wildcard-nsec3-optout.example A +norec +dnssec > dig.out.2.$n 2>&1 || ret=1
# Ensure that the AUTHORITY section contains an NS RRset without an associated
# DS RRset.
exactly_one_record_exists_for "delegation.wildcard-nsec3-optout.example." NS dig.out.2.$n || ret=1
no_records_exist_for "delegation.wildcard-nsec3-optout.example." DS dig.out.2.$n || ret=1
# Check NSEC3 records in the AUTHORITY section.
no_records_exist_for "2JGSPT59VJ7R9SQB5B9P6HPM5JBATOOO.wildcard-nsec3-optout.example." NSEC3 dig.out.2.$n || ret=1
no_records_exist_for "OKRFKC9SS1O60E8U2980UD62MUSMKGUG.wildcard-nsec3-optout.example." NSEC3 dig.out.2.$n || ret=1
exactly_one_record_exists_for "SS5M1RUBSGMANEQ1VLRDDEC6SOAT7HNI.wildcard-nsec3-optout.example." NSEC3 dig.out.2.$n || ret=1
# Ensure the NSEC3 record covering the zone cut does not have the DS bit set in
# the type bit map.
ensure_no_ds_in_bitmap "SS5M1RUBSGMANEQ1VLRDDEC6SOAT7HNI.wildcard-nsec3-optout.example." NSEC3 dig.out.2.$n || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking CNAME to DNAME from authoritative ($n)"
ret=0
$DIG $DIGOPTS cname.example @10.53.0.2 a > dig.out.ns2.cname
grep "status: NOERROR" dig.out.ns2.cname > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking CNAME to DNAME from recursive"
ret=0
$RNDCCMD 10.53.0.7 null --- start test$n --- 2>&1 | sed 's/^/ns7 /' | cat_i
$DIG $DIGOPTS cname.example @10.53.0.7 a > dig.out.ns4.cname
grep "status: NOERROR" dig.out.ns4.cname > /dev/null || ret=1
grep '^cname.example.' dig.out.ns4.cname > /dev/null || ret=1
grep '^cnamedname.example.' dig.out.ns4.cname > /dev/null || ret=1
grep '^a.cnamedname.example.' dig.out.ns4.cname > /dev/null || ret=1
grep '^a.target.example.' dig.out.ns4.cname > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking DNAME is returned with synthesized CNAME before DNAME ($n)"
ret=0
$RNDCCMD 10.53.0.7 null --- start test$n --- 2>&1 | sed 's/^/ns7 /' | cat_i
$DIG $DIGOPTS @10.53.0.7 name.synth-then-dname.example.broken A > dig.out.test$n
grep "status: NXDOMAIN" dig.out.test$n > /dev/null || ret=1
grep '^name.synth-then-dname\.example\.broken\..*CNAME.*name.$' dig.out.test$n > /dev/null || ret=1
grep '^synth-then-dname\.example\.broken\..*DNAME.*\.$' dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking DNAME is returned with CNAME to synthesized CNAME before DNAME ($n)"
ret=0
$RNDCCMD 10.53.0.7 null --- start test$n --- 2>&1 | sed 's/^/ns7 /' | cat_i
$DIG $DIGOPTS @10.53.0.7 cname-to-synth2-then-dname.example.broken A > dig.out.test$n
grep "status: NXDOMAIN" dig.out.test$n > /dev/null || ret=1
grep '^cname-to-synth2-then-dname\.example\.broken\..*CNAME.*name\.synth2-then-dname\.example\.broken.$' dig.out.test$n > /dev/null || ret=1
grep '^name\.synth2-then-dname\.example\.broken\..*CNAME.*name.$' dig.out.test$n > /dev/null || ret=1
grep '^synth2-then-dname\.example\.broken\..*DNAME.*\.$' dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking CNAME loops are detected ($n)"
ret=0
$RNDCCMD 10.53.0.7 null --- start test$n --- 2>&1 | sed 's/^/ns7 /' | cat_i
$DIG $DIGOPTS @10.53.0.7 loop.example > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 17" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking CNAME to external delegated zones is handled ($n)"
ret=0
$RNDCCMD 10.53.0.7 null --- start test$n --- 2>&1 | sed 's/^/ns7 /' | cat_i
$DIG $DIGOPTS @10.53.0.7 a.example > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 2" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking CNAME to internal delegated zones is handled ($n)"
ret=0
$RNDCCMD 10.53.0.7 null --- start test$n --- 2>&1 | sed 's/^/ns7 /' | cat_i
$DIG $DIGOPTS @10.53.0.7 b.example > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 2" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking CNAME to signed external delegation is handled ($n)"
ret=0
$RNDCCMD 10.53.0.7 null --- start test$n --- 2>&1 | sed 's/^/ns7 /' | cat_i
$DIG $DIGOPTS @10.53.0.7 c.example > dig.out.$n
grep "status: NOERROR" dig.out.$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking CNAME to signed internal delegation is handled ($n)"
ret=0
$RNDCCMD 10.53.0.7 null --- start test$n --- 2>&1 | sed 's/^/ns7 /' | cat_i
$DIG $DIGOPTS @10.53.0.7 d.example > dig.out.$n
grep "status: NOERROR" dig.out.$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking CNAME chains in various orders ($n)"
ret=0
$RNDCCMD 10.53.0.7 null --- start test$n - step 1 --- 2>&1 | sed 's/^/ns7 /' | cat_i
echo "cname,cname,cname|1,2,3,4,s1,s2,s3,s4" | $SEND
$DIG $DIGOPTS @10.53.0.7 test.domain.nil > dig.out.1.$n 2>&1
grep 'status: NOERROR' dig.out.1.$n > /dev/null 2>&1 || ret=1
grep 'ANSWER: 2' dig.out.1.$n > /dev/null 2>&1 || ret=1
$RNDCCMD 10.53.0.7 null --- start test$n - step 2 --- 2>&1 | sed 's/^/ns7 /' | cat_i
$RNDCCMD 10.53.0.7 flush 2>&1 | sed 's/^/ns7 /' | cat_i
echo "cname,cname,cname|1,1,2,2,3,4,s4,s3,s1" | $SEND
$DIG $DIGOPTS @10.53.0.7 test.domain.nil > dig.out.2.$n 2>&1
grep 'status: NOERROR' dig.out.2.$n > /dev/null 2>&1 || ret=1
grep 'ANSWER: 2' dig.out.2.$n > /dev/null 2>&1 || ret=1
$RNDCCMD 10.53.0.7 null --- start test$n - step 3 --- 2>&1 | sed 's/^/ns7 /' | cat_i
$RNDCCMD 10.53.0.7 flush 2>&1 | sed 's/^/ns7 /' | cat_i
echo "cname,cname,cname|2,1,3,4,s3,s1,s2,s4" | $SEND
$DIG $DIGOPTS @10.53.0.7 test.domain.nil > dig.out.3.$n 2>&1
grep 'status: NOERROR' dig.out.3.$n > /dev/null 2>&1 || ret=1
grep 'ANSWER: 2' dig.out.3.$n > /dev/null 2>&1 || ret=1
$RNDCCMD 10.53.0.7 null --- start test$n - step 4 --- 2>&1 | sed 's/^/ns7 /' | cat_i
$RNDCCMD 10.53.0.7 flush 2>&1 | sed 's/^/ns7 /' | cat_i
echo "cname,cname,cname|4,3,2,1,s4,s3,s2,s1" | $SEND
$DIG $DIGOPTS @10.53.0.7 test.domain.nil > dig.out.4.$n 2>&1
grep 'status: NOERROR' dig.out.4.$n > /dev/null 2>&1 || ret=1
grep 'ANSWER: 2' dig.out.4.$n > /dev/null 2>&1 || ret=1
echo "cname,cname,cname|4,3,2,1,s4,s3,s2,s1" | $SEND
$RNDCCMD 10.53.0.7 null --- start test$n - step 5 --- 2>&1 | sed 's/^/ns7 /' | cat_i
$RNDCCMD 10.53.0.7 flush 2>&1 | sed 's/^/ns7 /' | cat_i
$DIG $DIGOPTS @10.53.0.7 test.domain.nil > dig.out.5.$n 2>&1
grep 'status: NOERROR' dig.out.5.$n > /dev/null 2>&1 || ret=1
grep 'ANSWER: 2' dig.out.5.$n > /dev/null 2>&1 || ret=1
$RNDCCMD 10.53.0.7 null --- start test$n - step 6 --- 2>&1 | sed 's/^/ns7 /' | cat_i
$RNDCCMD 10.53.0.7 flush 2>&1 | sed 's/^/ns7 /' | cat_i
echo "cname,cname,cname|4,3,3,3,s1,s1,1,3,4" | $SEND
$DIG $DIGOPTS @10.53.0.7 test.domain.nil > dig.out.6.$n 2>&1
grep 'status: NOERROR' dig.out.6.$n > /dev/null 2>&1 || ret=1
grep 'ANSWER: 2' dig.out.6.$n > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that only the initial CNAME is cached ($n)"
ret=0
$RNDCCMD 10.53.0.7 flush 2>&1 | sed 's/^/ns7 /' | cat_i
echo "cname,cname,cname|1,2,3,4,s1,s2,s3,s4" | $SEND
$RNDCCMD 10.53.0.7 null --- start test$n --- 2>&1 | sed 's/^/ns7 /' | cat_i
$DIG $DIGOPTS @10.53.0.7 test.domain.nil > dig.out.1.$n 2>&1
sleep 1
$DIG $DIGOPTS +noall +answer @10.53.0.7 cname1.domain.nil > dig.out.2.$n 2>&1
ttl=`awk '{print $2}' dig.out.2.$n`
[ "$ttl" -eq 86400 ] || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking DNAME chains in various orders ($n)"
ret=0
$RNDCCMD 10.53.0.7 null --- start test$n - step 1 --- 2>&1 | sed 's/^/ns7 /' | cat_i
$RNDCCMD 10.53.0.7 flush 2>&1 | sed 's/^/ns7 /' | cat_i
echo "dname,dname|5,4,3,2,1,s5,s4,s3,s2,s1" | $SEND
$DIG $DIGOPTS @10.53.0.7 test.domain.nil > dig.out.1.$n 2>&1
grep 'status: NOERROR' dig.out.1.$n > /dev/null 2>&1 || ret=1
grep 'ANSWER: 3' dig.out.1.$n > /dev/null 2>&1 || ret=1
$RNDCCMD 10.53.0.7 null --- start test$n - step 2 --- 2>&1 | sed 's/^/ns7 /' | cat_i
$RNDCCMD 10.53.0.7 flush 2>&1 | sed 's/^/ns7 /' | cat_i
echo "dname,dname|5,4,3,2,1,s5,s4,s3,s2,s1" | $SEND
$DIG $DIGOPTS @10.53.0.7 test.domain.nil > dig.out.2.$n 2>&1
grep 'status: NOERROR' dig.out.2.$n > /dev/null 2>&1 || ret=1
grep 'ANSWER: 3' dig.out.2.$n > /dev/null 2>&1 || ret=1
$RNDCCMD 10.53.0.7 null --- start test$n - step 3 --- 2>&1 | sed 's/^/ns7 /' | cat_i
$RNDCCMD 10.53.0.7 flush 2>&1 | sed 's/^/ns7 /' | cat_i
echo "dname,dname|2,3,s1,s2,s3,s4,1" | $SEND
$DIG $DIGOPTS @10.53.0.7 test.domain.nil > dig.out.3.$n 2>&1
grep 'status: NOERROR' dig.out.3.$n > /dev/null 2>&1 || ret=1
grep 'ANSWER: 3' dig.out.3.$n > /dev/null 2>&1 || ret=1
$RNDCCMD 10.53.0.7 flush 2>&1 | sed 's/^/ns7 /' | cat_i
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking external CNAME/DNAME chains in various orders ($n)"
ret=0
$RNDCCMD 10.53.0.7 null --- start test$n - step 1 --- 2>&1 | sed 's/^/ns7 /' | cat_i
echo "xname,dname|1,2,3,4,s1,s2,s3,s4" | $SEND
$DIG $DIGOPTS @10.53.0.7 test.domain.nil > dig.out.1.$n 2>&1
grep 'status: NOERROR' dig.out.1.$n > /dev/null 2>&1 || ret=1
grep 'ANSWER: 2' dig.out.1.$n > /dev/null 2>&1 || ret=1
$RNDCCMD 10.53.0.7 null --- start test$n - step 2 --- 2>&1 | sed 's/^/ns7 /' | cat_i
$RNDCCMD 10.53.0.7 flush 2>&1 | sed 's/^/ns7 /' | cat_i
echo "xname,dname|s2,2,s1,1,4,s4,3" | $SEND
$DIG $DIGOPTS @10.53.0.7 test.domain.nil > dig.out.2.$n 2>&1
grep 'status: NOERROR' dig.out.2.$n > /dev/null 2>&1 || ret=1
grep 'ANSWER: 2' dig.out.2.$n > /dev/null 2>&1 || ret=1
$RNDCCMD 10.53.0.7 null --- start test$n - step 3 --- 2>&1 | sed 's/^/ns7 /' | cat_i
$RNDCCMD 10.53.0.7 flush 2>&1 | sed 's/^/ns7 /' | cat_i
echo "xname,dname|s2,2,2,2" | $SEND
$DIG $DIGOPTS @10.53.0.7 test.domain.nil > dig.out.3.$n 2>&1
grep 'status: SERVFAIL' dig.out.3.$n > /dev/null 2>&1 || ret=1
$RNDCCMD 10.53.0.7 flush 2>&1 | sed 's/^/ns7 /' | cat_i
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking explicit DNAME query ($n)"
ret=0
$RNDCCMD 10.53.0.7 null --- start test$n --- 2>&1 | sed 's/^/ns7 /' | cat_i
$DIG $DIGOPTS @10.53.0.7 dname short-dname.example > dig.out.7.$n 2>&1
grep 'status: NOERROR' dig.out.7.$n > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking DNAME via ANY query ($n)"
ret=0
$RNDCCMD 10.53.0.7 null --- start test$n --- 2>&1 | sed 's/^/ns7 /' | cat_i
$RNDCCMD 10.53.0.7 flush 2>&1 | sed 's/^/ns7 /' | cat_i
$DIG $DIGOPTS @10.53.0.7 any short-dname.example > dig.out.7.$n 2>&1
grep 'status: NOERROR' dig.out.7.$n > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Regression test for CVE-2021-25215 (authoritative server).
n=`expr $n + 1`
echo_i "checking DNAME resolution via itself (authoritative) ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.2 DNAME self.domain0.self.domain0.nil. > dig.out.2.$n 2>&1
grep 'status: NOERROR' dig.out.2.$n > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Regression test for CVE-2021-25215 (recursive resolver).
n=`expr $n + 1`
echo_i "checking DNAME resolution via itself (recursive) ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.7 DNAME self.example.self.example.dname. > dig.out.7.$n 2>&1
grep 'status: NOERROR' dig.out.7.$n > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
