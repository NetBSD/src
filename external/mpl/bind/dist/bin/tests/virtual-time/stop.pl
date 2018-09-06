#!/usr/bin/perl -w
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

# Framework for stopping test servers
# Based on the type of server specified, signal the server to stop, wait
# briefly for it to die, and then kill it if it is still alive.
# If a server is specified, stop it. Otherwise, stop all servers for test.

use strict;
use Cwd 'abs_path';

# Option handling
#   [--use-rndc] test [server]
#
#   test - name of the test directory
#   server - name of the server directory

my $usage = "usage: $0 [--use-rndc] test-directory [server-directory]";
my $use_rndc;

while (@ARGV && $ARGV[0] =~ /^-/) {
	my $opt = shift @ARGV;
	if ($opt eq '--use-rndc') {
		$use_rndc = 1;
	} else {
		die "$usage\n";
	}
}

my $test = $ARGV[0];
my $server = $ARGV[1];

my $errors = 0;

die "$usage\n" unless defined($test);
die "No test directory: \"$test\"\n" unless (-d $test);
die "No server directory: \"$server\"\n" if (defined($server) && !-d "$test/$server");
    
# Global variables
my $testdir = abs_path($test);
my @servers;


# Determine which servers need to be stopped.
if (defined $server) {
	@servers = ($server);
} else {
	local *DIR;
	opendir DIR, $testdir or die "$testdir: $!\n";
	my @files = sort readdir DIR;
	closedir DIR;

	my @ns = grep /^ns[0-9]*$/, @files;
	
	push @servers, @ns;
}


# Stop the server(s), pass 1: rndc.
if ($use_rndc) {
	foreach my $server (grep /^ns/, @servers) {
		stop_rndc($server);
	}

	wait_for_servers(30, grep /^ns/, @servers);
}


# Pass 2: SIGTERM
foreach my $server (@servers) {
	stop_signal($server, "TERM");
}

wait_for_servers(60, @servers);

# Pass 3: SIGABRT
foreach my $server (@servers) {
	stop_signal($server, "ABRT");
}

exit($errors ? 1 : 0);

# Subroutines

# Return the full path to a given server's PID file.
sub server_pid_file {
	my($server) = @_;

	my $pid_file;
	if ($server =~ /^ns/) {
		$pid_file = "named.pid";
	} else {
		print "I:Unknown server type $server\n";
		exit 1;
	}
	$pid_file = "$testdir/$server/$pid_file";
}

# Read a PID.
sub read_pid {
	my($pid_file) = @_;

	local *FH;
	my $result = open FH, "< $pid_file";
	if (!$result) {
		print "I:$pid_file: $!\n";
		unlink $pid_file;
		return;
	}

	my $pid = <FH>;
	chomp($pid);
	return $pid;
}

# Stop a named process with rndc.
sub stop_rndc {
	my($server) = @_;

	return unless ($server =~ /^ns(\d+)$/);
	my $ip = "10.53.0.$1";

	# Ugly, but should work.
	system("$ENV{RNDC} -c $testdir/../common/rndc.conf -s $ip -p 9953 stop | sed 's/^/I:$server /'");
	return;
}

# Stop a server by sending a signal to it.
sub stop_signal {
	my($server, $sig) = @_;

	my $pid_file = server_pid_file($server);
	return unless -f $pid_file;
	
	my $pid = read_pid($pid_file);
	return unless defined($pid);

	if ($sig eq 'ABRT') {
		print "I:$server didn't die when sent a SIGTERM\n";
		$errors++;
	}

	my $result = kill $sig, $pid;
	if (!$result) {
		print "I:$server died before a SIG$sig was sent\n";
		unlink $pid_file;
		$errors++;
	}

	return;
}

sub wait_for_servers {
	my($timeout, @servers) = @_;

	my @pid_files = grep { defined($_) }
	                map  { server_pid_file($_) } @servers;

	while ($timeout > 0 && @pid_files > 0) {
		@pid_files = grep { -f $_ } @pid_files;
		sleep 1 if (@pid_files > 0);
		$timeout--;
	}

	return;
}
