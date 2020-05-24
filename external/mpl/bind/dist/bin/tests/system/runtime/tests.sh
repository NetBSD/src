#!/bin/sh
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

# shellcheck source=conf.sh
SYSTEMTESTTOP=..
. "$SYSTEMTESTTOP/conf.sh"

set -e

RNDCCMD="$RNDC -c $SYSTEMTESTTOP/common/rndc.conf -p ${CONTROLPORT} -s"

kill_named() {
	pidfile="${1}"
	if [ ! -r "${pidfile}" ]; then
		return 1
	fi

	pid=$(cat "${pidfile}" 2>/dev/null)
	if [ "${pid:+set}" = "set" ]; then
		$KILL -15 "${pid}" >/dev/null 2>&1
		retries=10
		while [ "$retries" -gt 0 ]; do
			if ! $KILL -0 "${pid}" >/dev/null 2>&1; then
				break
			fi
			sleep 1
			retries=$((retries-1))
		done
		# Timed-out
		if [ "$retries" -eq 0 ]; then
			echo_i "failed to kill named ($pidfile)"
			return 1
		fi
	fi
	rm -f "${pidfile}"
	return 0
}

wait_for_named() {
	retries=10
	while [ "$retries" -gt 0 ]; do
		if grep "$@" >/dev/null 2>&1; then
			break
		fi
		sleep 1
		retries=$((retries-1))
	done
	if [ "$retries" -eq 0 ]; then
		return 1
	fi
	return 0
}

status=0
n=0

n=$((n+1))
echo_i "verifying that named started normally ($n)"
ret=0
[ -s ns2/named.pid ] || ret=1
grep "unable to listen on any configured interface" ns2/named.run > /dev/null && ret=1
grep "another named process" ns2/named.run > /dev/null && ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "verifying that named checks for conflicting named processes ($n)"
ret=0
(cd ns2 && $NAMED -c named-alt2.conf -D runtime-ns2-extra-2 -X named.lock -m record,size,mctx -d 99 -g -U 4 >> named$n.run 2>&1 & )
wait_for_named "another named process" ns2/named$n.run || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "verifying that 'lock-file none' disables process check ($n)"
ret=0
(cd ns2 && $NAMED -c named-alt3.conf -D runtime-ns2-extra-3 -m record,size,mctx -d 99 -g -U 4 >> named$n.run 2>&1 & )
wait_for_named "running$" ns2/named$n.run || ret=1
grep "another named process" ns2/named$n.run > /dev/null && ret=1
kill_named ns2/named-alt3.pid || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that named refuses to reconfigure if working directory is not writable ($n)"
ret=0
copy_setports ns2/named-alt4.conf.in ns2/named.conf
$RNDCCMD 10.53.0.2 reconfig > rndc.out.$n 2>&1 && ret=1
grep "failed: permission denied" rndc.out.$n > /dev/null 2>&1 || ret=1
sleep 1
grep "[^-]directory './nope' is not writable" ns2/named.run > /dev/null 2>&1 || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that named refuses to reconfigure if managed-keys-directory is not writable ($n)"
ret=0
copy_setports ns2/named-alt5.conf.in ns2/named.conf
$RNDCCMD 10.53.0.2 reconfig > rndc.out.$n 2>&1 && ret=1
grep "failed: permission denied" rndc.out.$n > /dev/null 2>&1 || ret=1
sleep 1
grep "managed-keys-directory './nope' is not writable" ns2/named.run > /dev/null 2>&1 || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that named refuses to reconfigure if new-zones-directory is not writable ($n)"
ret=0
copy_setports ns2/named-alt6.conf.in ns2/named.conf
$RNDCCMD 10.53.0.2 reconfig > rndc.out.$n 2>&1 && ret=1
grep "failed: permission denied" rndc.out.$n > /dev/null 2>&1 || ret=1
sleep 1
grep "new-zones-directory './nope' is not writable" ns2/named.run > /dev/null 2>&1 || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that named recovers when configuration file is valid again ($n)"
ret=0
copy_setports ns2/named1.conf.in ns2/named.conf
$RNDCCMD 10.53.0.2 reconfig > rndc.out.$n 2>&1 || ret=1
[ -s ns2/named.pid ] || ret=1
kill_named ns2/named.pid || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that named refuses to start if working directory is not writable ($n)"
ret=0
(cd ns2 && $NAMED -c named-alt4.conf -D runtime-ns2-extra-4 -d 99 -g > named$n.run 2>&1 &)
wait_for_named "exiting (due to fatal error)" ns2/named$n.run || ret=1
grep "[^-]directory './nope' is not writable" ns2/named$n.run > /dev/null 2>&1 || ret=1
kill_named ns2/named.pid && ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that named refuses to start if managed-keys-directory is not writable ($n)"
ret=0
(cd ns2 && $NAMED -c named-alt5.conf -D runtime-ns2-extra-5 -d 99 -g > named$n.run 2>&1 &)
wait_for_named "exiting (due to fatal error)" ns2/named$n.run || ret=1
grep "managed-keys-directory './nope' is not writable" ns2/named$n.run > /dev/null 2>&1 || ret=1
kill_named named.pid && ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that named refuses to start if new-zones-directory is not writable ($n)"
ret=0
(cd ns2 && $NAMED -c named-alt6.conf -D runtime-ns2-extra-6 -d 99 -g > named$n.run 2>&1 &)
wait_for_named "exiting (due to fatal error)" ns2/named$n.run || ret=1
grep "new-zones-directory './nope' is not writable" ns2/named$n.run > /dev/null 2>&1 || ret=1
kill_named ns2/named.pid && ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that named logs control characters in octal notation ($n)"
ret=0
INSTANCE_NAME="runtime-ns2-extra-7-$(cat ctrl-chars)"
(cd ns2 && $NAMED -c named-alt7.conf -D "${INSTANCE_NAME}" -d 99 -g > named$n.run 2>&1 &)
wait_for_named "running$" ns2/named$n.run || ret=1
grep 'running as.*\\177\\033' ns2/named$n.run > /dev/null || ret=1
kill_named ns2/named.pid || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that named escapes special characters in the logs ($n)"
ret=0
INSTANCE_NAME="runtime-ns2-extra-8-$;"
(cd ns2 && $NAMED -c named-alt7.conf -D "${INSTANCE_NAME}" -d 99 -g > named$n.run 2>&1 &)
wait_for_named "running$" ns2/named$n.run || ret=1
grep 'running as.*\\$\\;' ns2/named$n.run > /dev/null || ret=1
kill_named ns2/named.pid || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that named logs an ellipsis when the command line is larger than 8k bytes ($n)"
ret=0
LONG_CMD_LINE=$(cat long-cmd-line)
# shellcheck disable=SC2086
(cd ns2 && $NAMED $LONG_CMD_LINE -c "named-alt7.conf" -g > named$n.run 2>&1 &)
wait_for_named "running$" ns2/named$n.run || ret=1
grep "running as.*\.\.\.$" ns2/named$n.run > /dev/null || ret=1
kill_named ns2/named.pid || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "verifying that named switches UID ($n)"
if [ "$(id -u)" -eq 0 ] && [ -z "$CYGWIN" ]; then
    ret=0
    TEMP_NAMED_DIR=$(mktemp -d "$(pwd)/ns2/tmp.XXXXXXXX")
    if [ "$?" -eq 0 ]; then
        copy_setports ns2/named-alt9.conf.in "${TEMP_NAMED_DIR}/named-alt9.conf"
        export SOFTHSM2_CONF="${TEMP_NAMED_DIR}/softhsm2.conf"
        sh "$TOP/bin/tests/prepare-softhsm2.sh"
        chown -R nobody: "${TEMP_NAMED_DIR}"
        chmod 0700 "${TEMP_NAMED_DIR}"
        ( cd "${TEMP_NAMED_DIR}" && $NAMED -u nobody -c named-alt9.conf -d 99 -g -U 4 >> named$n.run 2>&1 & ) || ret=1
        wait_for_named "running$" "${TEMP_NAMED_DIR}/named$n.run" || ret=1
        [ -s "${TEMP_NAMED_DIR}/named9.pid" ] || ret=1
        grep "loading configuration: permission denied" "${TEMP_NAMED_DIR}/named$n.run" > /dev/null && ret=1
        kill_named "${TEMP_NAMED_DIR}/named9.pid" || ret=1
    else
        echo_i "mktemp failed"
        ret=1
    fi
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status+ret))
else
    echo_i "skipped, not running as root or running on Windows"
fi

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
