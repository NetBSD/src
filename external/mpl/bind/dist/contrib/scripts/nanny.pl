#!/usr/bin/perl
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

# A simple nanny to make sure named stays running.

$pid_file_location = '/var/run/named.pid';
$nameserver_location = 'localhost';
$dig_program = 'dig';
$named_program =  'named';

fork() && exit();

for (;;) {
	$pid = 0;
	open(FILE, $pid_file_location) || goto restart;
	$pid = <FILE>;
	close(FILE);
	chomp($pid);

	$res = kill 0, $pid;

	goto restart if ($res == 0);

	$dig_command =
	       "$dig_program +short . \@$nameserver_location > /dev/null";
	$return = system($dig_command);
	goto restart if ($return == 9);

	sleep 30;
	next;

 restart:
	if ($pid != 0) {
		kill 15, $pid;
		sleep 30;
	}
	system ($named_program);
	sleep 120;
}
