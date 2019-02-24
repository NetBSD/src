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
use warnings;

use Cwd ':DEFAULT', 'abs_path';
use English '-no_match_vars';
use Getopt::Long;

# Usage:
#   perl stop.pl [--use-rndc [--port port]] test [server]
#
#   --use-rndc      Attempt to stop the server via the "rndc stop" command.
#
#   --port port     Only relevant if --use-rndc is specified, this sets the
#                   command port over which the attempt should be made.  If
#                   not specified, port 9953 is used.
#
#   test            Name of the test directory.
#
#   server          Name of the server directory.

my $usage = "usage: $0 [--use-rndc [--halt] [--port port]] test-directory [server-directory]";

my $use_rndc = 0;
my $halt = 0;
my $rndc_port = 9953;
my $errors = 0;

GetOptions(
	'use-rndc!' => \$use_rndc,
	'halt!' => \$halt,
	'port=i' => \$rndc_port
    ) or die "$usage\n";

my ( $test, $server_arg ) = @ARGV;

if (!$test) {
	die "$usage\n";
}

# Global variables
my $topdir = abs_path($ENV{'SYSTEMTESTTOP'});
my $testdir = abs_path($topdir . "/" . $test);

if (! -d $testdir) {
	die "No test directory: \"$testdir\"\n";
}

if ($server_arg && ! -d "$testdir/$server_arg") {
	die "No server directory: \"$testdir/$server_arg\"\n";
}

my $RNDC = $ENV{RNDC};

my @ns;
my @ans;

if ($server_arg) {
	if ($server_arg =~ /^ns/) {
		push(@ns, $server_arg);
	} elsif ($server_arg =~ /^ans/) {
		push(@ans, $server_arg);
	} else {
		print "$0: ns or ans directory expected";
		print "I:$test:failed";
	}
} else {
	# Determine which servers need to be stopped for this test.
	opendir DIR, $testdir or die "unable to read test directory: \"$test\" ($OS_ERROR)\n";
	my @files = sort readdir DIR;
	closedir DIR;

	@ns = grep /^ns[0-9]*$/, @files;
	@ans = grep /^ans[0-9]*$/, @files;
}

# Stop the server(s), pass 1: rndc.
if ($use_rndc) {
	foreach my $name(@ns) {
		stop_rndc($name, $rndc_port);
	}

	@ns = wait_for_servers(30, @ns);
}

# Pass 2: SIGTERM
foreach my $name (@ns) {
	stop_signal($name, "TERM");
}

@ns = wait_for_servers(60, @ns);

foreach my $name(@ans) {
	stop_signal($name, "TERM", 1);
}

@ans = wait_for_servers(60, @ans);

# Pass 3: SIGABRT
foreach my $name (@ns) {
	print "I:$test:$name didn't die when sent a SIGTERM\n";
	stop_signal($name, "ABRT");
	$errors = 1;
}
foreach my $name (@ans) {
	print "I:$test:$name didn't die when sent a SIGTERM\n";
	stop_signal($name, "ABRT", 1);
	$errors = 1;
}

exit($errors);

# Subroutines

# Return the full path to a given server's PID file.
sub server_pid_file {
	my($server) = @_;

	return $testdir . "/" . $server . "/named.pid" if ($server =~ /^ns/);
	return $testdir . "/" . $server . "/ans.pid" if ($server =~ /^ans/);

	die "Unknown server type $server\n";
}

# Read a PID.
sub read_pid {
	my ( $pid_file ) = @_;

	return unless -f $pid_file;
	# we don't really care about the race condition here
	my $result = open(my $fh, "<", $pid_file);
	if (!defined($result)) {
		print "I:$test:$pid_file: $!\n";
		unlink $pid_file;
		return;
	}

	my $pid = <$fh>;
	return unless defined($pid);

	chomp($pid);
	return $pid;
}

# Stop a named process with rndc.
sub stop_rndc {
	my ( $server, $port ) = @_;
	my $n;

	if ($server =~ /^ns(\d+)/) {
		$n = $1;
	} else {
		die "unable to parse server number from name \"$server\"\n";
	}

	my $ip = "10.53.0.$n";
	my $how = $halt ? "halt" : "stop";

	# Ugly, but should work.
	system("$RNDC -c ../common/rndc.conf -s $ip -p $port $how | sed 's/^/I:$test:$server /'");
	return;
}

sub server_died {
	my ( $server, $signal ) = @_;
	
	print "I:$test:$server died before a SIG$signal was sent\n";
	$errors = 1;

	my $pid_file = server_pid_file($server);
	unlink($pid_file);

	return;
}

sub send_signal {
	my ( $signal, $pid, $ans ) = @_;

	if (! defined $ans) {
		$ans = 0;
	}

	my $result = 0;

	if (!$ans && ($^O eq 'cygwin' || $^O eq 'msys')) {
		my $killout = `/bin/kill -f -$signal $pid 2>&1`;
		chomp($killout);
		$result = 1 if ($killout eq '');
	} else {
		$result = kill $signal, $pid;
	}
	return $result;
}

# Stop a server by sending a signal to it.
sub stop_signal {
	my ( $server, $signal, $ans ) = @_;
	if (! defined $ans) {
		$ans = 0;
	}

	my $pid_file = server_pid_file($server);
	my $pid = read_pid($pid_file);

	return unless defined($pid);

	# Send signal to the server, and bail out if signal can't be sent
	if (send_signal($signal, $pid, $ans) != 1) {
		server_died($server, $signal);
		return;
	}

	return;
}

sub clean_pid_file {
	my ( $server ) = @_;

	my $pid_file = server_pid_file($server);
	my $pid = read_pid($pid_file);

	return unless defined($pid);

	# If we're here, the PID file hasn't been cleaned up yet
	if (send_signal(0, $pid) == 0) {
		# XXX: on windows this is likely to result in a
		# false positive, so don't bother reporting the error.
		if ($ENV{'CYGWIN'} eq "") {
			print "I:$test:$server crashed on shutdown\n";
			$errors = 1;
		}
		return;
	}

	return $server;
}

sub wait_for_servers {
	my ( $timeout, @servers ) = @_;

	while ($timeout > 0 && @servers > 0) {
		sleep 1 if (@servers > 0);
		@servers =
			grep { defined($_) }
			map  { clean_pid_file($_) } @servers;
		$timeout--;
	}

	return @servers;
}
