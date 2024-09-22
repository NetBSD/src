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

set -e

. ../conf.sh

status=0

echo_i "test name too long"
$PERL formerr.pl -a 10.53.0.1 -p ${PORT} nametoolong >nametoolong.out
ans=$(grep got: nametoolong.out)
if [ "${ans}" != "got: 000080010000000000000000" ]; then
  echo_i "failed"
  status=$((status + 1))
fi

echo_i "two question names"
$PERL formerr.pl -a 10.53.0.1 -p ${PORT} twoquestionnames >twoquestionnames.out
ans=$(grep got: twoquestionnames.out)
if [ "${ans}" != "got: 000080010000000000000000" ]; then
  echo_i "failed"
  status=$((status + 1))
fi

echo_i "two question types"
$PERL formerr.pl -a 10.53.0.1 -p ${PORT} twoquestiontypes >twoquestiontypes.out
ans=$(grep got: twoquestiontypes.out)
if [ "${ans}" != "got: 0000800100020000000000000e41414141414141414141414141410000010001c00c00020001" ]; then
  echo_i "failed"
  status=$((status + 1))
fi

echo_i "duplicate questions"
$PERL formerr.pl -a 10.53.0.1 -p ${PORT} dupquestion >dupquestion.out
ans=$(grep got: dupquestion.out)
if [ "${ans}" != "got: 000080010000000000000000" ]; then
  echo_i "failed"
  status=$(expr $status + 1)
fi

echo_i "duplicate answer"
$PERL formerr.pl -a 10.53.0.1 -p ${PORT} dupans >dupans.out
ans=$(grep got: dupans.out)
if [ "${ans}" != "got: 0000800100010000000000000000060001" ]; then
  echo_i "failed"
  status=$(expr $status + 1)
fi

echo_i "question only type in answer"
$PERL formerr.pl -a 10.53.0.1 -p ${PORT} qtypeasanswer >qtypeasanswer.out
ans=$(grep got: qtypeasanswer.out)
if [ "${ans}" != "got: 000080010000000000000000" ]; then
  echo_i "failed"
  status=$(expr $status + 1)
fi

# this would be NOERROR if it included a COOKIE option,
# but is a FORMERR without one.
echo_i "empty question section (and no COOKIE option)"
$PERL formerr.pl -a 10.53.0.1 -p ${PORT} noquestions >noquestions.out
ans=$(grep got: noquestions.out)
if [ "${ans}" != "got: 000080010000000000000000" ]; then
  echo_i "failed"
  status=$((status + 1))
fi

echo_i "bad nsec3 owner"
$PERL formerr.pl -a 10.53.0.1 -p ${PORT} badnsec3owner >badnsec3owner.out
ans=$(grep got: badnsec3owner.out)
# SERVFAIL (2) rather than FORMERR (1)
if [ "${ans}" != "got: 0008800200010000000000000000010001" ]; then
  echo_i "failed"
  status=$(expr $status + 1)
fi

echo_i "short record before rdata "
$PERL formerr.pl -a 10.53.0.1 -p ${PORT} shortrecord >shortrecord.out
ans=$(grep got: shortrecord.out)
if [ "${ans}" != "got: 000980010000000000000000" ]; then
  echo_i "failed"
  status=$(expr $status + 1)
fi

echo_i "short question"
$PERL formerr.pl -a 10.53.0.1 -p ${PORT} shortquestion >shortquestion.out
ans=$(grep got: shortquestion.out)
if [ "${ans}" != "got: 000080010000000000000000" ]; then
  echo_i "failed"
  status=$(expr $status + 1)
fi

echo_i "mismatch classes in question section"
$PERL formerr.pl -a 10.53.0.1 -p ${PORT} questionclass >questionclass.out
ans=$(grep got: questionclass.out)
if [ "${ans}" != "got: 000080010000000000000000" ]; then
  echo_i "failed"
  status=$(expr $status + 1)
fi

echo_i "bad record owner name"
$PERL formerr.pl -a 10.53.0.1 -p ${PORT} badrecordname >badrecordname.out
ans=$(grep got: badrecordname.out)
if [ "${ans}" != "got: 000080010000000000000000" ]; then
  echo_i "failed"
  status=$(expr $status + 1)
fi

echo_i "mismatched class in record"
$PERL formerr.pl -a 10.53.0.1 -p ${PORT} wrongclass >wrongclass.out
ans=$(grep got: wrongclass.out)
if [ "${ans}" != "got: 000080010000000000000000" ]; then
  echo_i "failed"
  status=$(expr $status + 1)
fi

echo_i "mismatched KEY class"
$PERL formerr.pl -a 10.53.0.1 -p ${PORT} keyclass >keyclass.out
ans=$(grep got: keyclass.out)
if [ "${ans}" != "got: 0000800100010000000000000000010001" ]; then
  echo_i "failed"
  status=$(expr $status + 1)
fi

echo_i "OPT wrong owner name"
$PERL formerr.pl -a 10.53.0.1 -p ${PORT} optwrongname >optwrongname.out
ans=$(grep got: optwrongname.out)
if [ "${ans}" != "got: 000080010000000000000000" ]; then
  echo_i "failed"
  status=$(expr $status + 1)
fi

echo_i "RRSIG invalid covers"
$PERL formerr.pl -a 10.53.0.1 -p ${PORT} malformedrrsig >malformedrrsig.out
ans=$(grep got: malformedrrsig.out)
if [ "${ans}" != "got: 000080010000000000000000" ]; then
  echo_i "failed"
  status=$(expr $status + 1)
fi

echo_i "UPDATE malformed delete type"
$PERL formerr.pl -a 10.53.0.1 -p ${PORT} malformeddeltype >malformeddeltype.out
ans=$(grep got: malformeddeltype.out)
if [ "${ans}" != "got: 0000a8010000000000000000" ]; then
  echo_i "failed"
  status=$(expr $status + 1)
fi

echo_i "TSIG wrong class"
$PERL formerr.pl -a 10.53.0.1 -p ${PORT} tsigwrongclass >tsigwrongclass.out
ans=$(grep got: tsigwrongclass.out)
if [ "${ans}" != "got: 000080010000000000000000" ]; then
  echo_i "failed"
  status=$(expr $status + 1)
fi

echo_i "TSIG not last"
$PERL formerr.pl -a 10.53.0.1 -p ${PORT} tsignotlast >tsignotlast.out
ans=$(grep got: tsignotlast.out)
if [ "${ans}" != "got: 000080010000000000000000" ]; then
  echo_i "failed"
  status=$(expr $status + 1)
fi

echo_i "exit status: $status"

[ $status -eq 0 ] || exit 1
