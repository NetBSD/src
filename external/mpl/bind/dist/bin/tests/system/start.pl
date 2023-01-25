#!/usr/bin/perl -w

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

# Framework for starting test servers.
# Based on the type of server specified, check for port availability, remove
# temporary files, start the server, and verify that the server is running.
# If a server is specified, start it. Otherwise, start all servers for test.

use strict;
use warnings;

use Cwd ':DEFAULT', 'abs_path';
use English '-no_match_vars';
use Getopt::Long;
use Time::HiRes 'sleep'; # allows sleeping fractional seconds

# Usage:
#   perl start.pl [--noclean] [--restart] [--port port] [--taskset cpus] test [server [options]]
#
#   --noclean       Do not cleanup files in server directory.
#
#   --restart       Indicate that the server is being restarted, so get the
#                   server to append output to an existing log file instead of
#                   starting a new one.
#
#   --port port     Specify the default port being used by the server to answer
#                   queries (default 5300).  This script will interrogate the
#                   server on this port to see if it is running. (Note: for
#                   "named" nameservers, this can be overridden by the presence
#                   of the file "named.port" in the server directory containing
#                   the number of the query port.)
#
#   --taskset cpus  Use taskset to signal which cpus can be used. For example
#                   cpus=fff0 means all cpus aexcept for 0, 1, 2, and 3 are
#                   eligible.
#
#   test            Name of the test directory.
#
#   server          Name of the server directory.  This will be of the form
#                   "nsN" or "ansN", where "N" is an integer between 1 and 8.
#                   If not given, the script will start all the servers in the
#                   test directory.
#
#   options         Alternate options for the server.
#
#                   NOTE: options must be specified with '-- "<option list>"',
#                   for instance: start.pl . ns1 -- "-c n.conf -d 43"
#
#                   ALSO NOTE: this variable will be filled with the contents
#                   of the first non-commented/non-blank line of args in a file
#                   called "named.args" in an ns*/ subdirectory. Only the FIRST
#                   non-commented/non-blank line is used (everything else in
#                   the file is ignored). If "options" is already set, then
#                   "named.args" is ignored.

my $usage = "usage: $0 [--noclean] [--restart] [--port <port>] [--taskset <cpus>] test-directory [server-directory [server-options]]";
my $clean = 1;
my $restart = 0;
my $queryport = 5300;
my $taskset = "";

GetOptions(
	'clean!'    => \$clean,
	'restart!'  => \$restart,
	'port=i'    => \$queryport,
	'taskset=s' => \$taskset,
) or die "$usage\n";

my( $test, $server_arg, $options_arg ) = @ARGV;

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

my $NAMED = $ENV{'NAMED'};
my $DIG = $ENV{'DIG'};
my $PERL = $ENV{'PERL'};
my $PYTHON = $ENV{'PYTHON'};

# Start the server(s)

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
	# Determine which servers need to be started for this test.
	opendir DIR, $testdir or die "unable to read test directory: \"$test\" ($OS_ERROR)\n";
	my @files = sort readdir DIR;
	closedir DIR;

	@ns = grep /^ns[0-9]*$/, @files;
	@ans = grep /^ans[0-9]*$/, @files;
}

# Start the servers we found.

foreach my $name(@ns) {
	my $instances_so_far = count_running_lines($name);
	&check_ns_port($name);
	&start_ns_server($name, $options_arg);
	&verify_ns_server($name, $instances_so_far);
}

foreach my $name(@ans) {
	&start_ans_server($name);
}

# Subroutines

sub read_ns_port {
	my ( $server ) = @_;
	my $port = $queryport;
	my $options = "";

	if ($server) {
		my $file = $testdir . "/" . $server . "/named.port";

		if (-e $file) {
			open(my $fh, "<", $file) or die "unable to read ports file \"$file\" ($OS_ERROR)";

			my $line = <$fh>;

			if ($line) {
				chomp $line;
				$port = $line;
			}
		}
	}
	return ($port);
}

sub check_ns_port {
	my ( $server ) = @_;
	my $options = "";
	my $port = read_ns_port($server);

	if ($server =~ /(\d+)$/) {
		$options = "-i $1";
	}

	my $tries = 0;

	while (1) {
		my $return = system("$PERL $topdir/testsock.pl -p $port $options");

		if ($return == 0) {
			last;
		}

		$tries++;

		if ($tries > 4) {
			print "$0: could not bind to server addresses, still running?\n";
			print "I:$test:server sockets not available\n";
			print "I:$test:failed\n";

			system("$PERL $topdir/stop.pl $test"); # Is this the correct behavior?

			exit 1;
		}

		print "I:$test:Couldn't bind to socket (yet)\n";
		sleep 2;
	}
}

sub start_server {
	my ( $server, $command, $pid_file ) = @_;

	chdir "$testdir/$server" or die "unable to chdir \"$testdir/$server\" ($OS_ERROR)\n";

	# start the server
	my $child = `$command`;
	chomp($child);

	# wait up to 14 seconds for the server to start and to write the
	# pid file otherwise kill this server and any others that have
	# already been started
	my $tries = 0;
	while (!-s $pid_file) {
		if (++$tries > 140) {
			print "I:$test:Couldn't start server $command (pid=$child)\n";
			print "I:$test:failed\n";
			kill "ABRT", $child if ("$child" ne "");
			chdir "$testdir";
			system "$PERL $topdir/stop.pl $test";
			exit 1;
		}
		sleep 0.1;
	}

	# go back to the top level directory
	chdir $topdir;
}

sub construct_ns_command {
	my ( $server, $options ) = @_;

	my $command;

	if ($ENV{'USE_VALGRIND'}) {
		$command = "valgrind -q --gen-suppressions=all --num-callers=48 --fullpath-after= --log-file=named-$server-valgrind-%p.log ";

		if ($ENV{'USE_VALGRIND'} eq 'helgrind') {
			$command .= "--tool=helgrind ";
		} else {
			$command .= "--tool=memcheck --track-origins=yes --leak-check=full ";
		}

		$command .= "$NAMED -m none -M external ";
	} else {
		if ($taskset) {
			$command = "taskset $taskset $NAMED ";
		} else {
			$command = "$NAMED ";
		}
	}

	my $args_file = $testdir . "/" . $server . "/" . "named.args";

	if ($options) {
		$command .= $options;
	} elsif (-e $args_file) {
		open(my $fh, "<", $args_file) or die "unable to read args_file \"$args_file\" ($OS_ERROR)\n";

		while(my $line=<$fh>) {
			next if ($line =~ /^\s*$/); #discard blank lines
			next if ($line =~ /^\s*#/); #discard comment lines

			chomp $line;

			$line =~ s/#.*$//;

			$command .= $line;

			last;
		}
	} else {
		$command .= "-D $test-$server ";
		$command .= "-X named.lock ";
		$command .= "-m record,size,mctx ";

		foreach my $t_option(
			"dropedns", "ednsformerr", "ednsnotimp", "ednsrefused",
			"noaa", "noedns", "nosoa", "maxudp512", "maxudp1460",
		    ) {
			if (-e "$testdir/$server/named.$t_option") {
				$command .= "-T $t_option "
			}
		}

		$command .= "-c named.conf -d 99 -g -U 4 -T maxcachesize=2097152";
	}

	if (-e "$testdir/$server/named.notcp") {
		$command .= " -T notcp"
	}

	if ($restart) {
		$command .= " >>named.run 2>&1 &";
	} else {
		$command .= " >named.run 2>&1 &";
	}

	# get the shell to report the pid of the server ($!)
	$command .= " echo \$!";

	return $command;
}

sub start_ns_server {
	my ( $server, $options ) = @_;

	my $cleanup_files;
	my $command;
	my $pid_file;

	$cleanup_files = "{./*.jnl,./*.bk,./*.st,./named.run}";

	$command = construct_ns_command($server, $options);

	$pid_file = "named.pid";

	if ($clean) {
		unlink glob $cleanup_files;
	}

	start_server($server, $command, $pid_file);
}

sub construct_ans_command {
	my ( $server, $options ) = @_;

	my $command;
	my $n;

	if ($server =~ /^ans(\d+)/) {
		$n = $1;
	} else {
		die "unable to parse server number from name \"$server\"\n";
	}

	if (-e "$testdir/$server/ans.py") {
		$command = "$PYTHON -u ans.py 10.53.0.$n $queryport";
	} elsif (-e "$testdir/$server/ans.pl") {
		$command = "$PERL ans.pl";
	} else {
		$command = "$PERL $topdir/ans.pl 10.53.0.$n";
	}

	if ($options) {
		$command .= $options;
	}

	if ($restart) {
		$command .= " >>ans.run 2>&1 &";
	} else {
		$command .= " >ans.run 2>&1 &";
	}

	# get the shell to report the pid of the server ($!)
	$command .= " echo \$!";

	return $command;
}

sub start_ans_server {
	my ( $server, $options ) = @_;

	my $cleanup_files;
	my $command;
	my $pid_file;

	$cleanup_files = "{./ans.run}";
	$command = construct_ans_command($server, $options);
	$pid_file = "ans.pid";

	if ($clean) {
		unlink glob $cleanup_files;
	}

	start_server($server, $command, $pid_file);
}

sub count_running_lines {
	my ( $server ) = @_;

	my $runfile = "$testdir/$server/named.run";

	# the shell *ought* to have created the file immediately, but this
	# logic allows the creation to be delayed without issues
	if (open(my $fh, "<", $runfile)) {
		# the two non-whitespace blobs should be the date and time
		# but we don't care about them really, only that they are there
		return scalar(grep /^\S+ \S+ running\R/, <$fh>);
	} else {
		return 0;
	}
}

sub verify_ns_server {
	my ( $server, $instances_so_far ) = @_;

	my $tries = 0;

	while (count_running_lines($server) < $instances_so_far + 1) {
		$tries++;

		if ($tries >= 30) {
			print "I:$test:server $server seems to have not started\n";
			print "I:$test:failed\n";

			system("$PERL $topdir/stop.pl $test");

			exit 1;
		}

		sleep 2;
	}

	$tries = 0;

	my $port = read_ns_port($server);
	my $tcp = "+tcp";
	my $n;

	if ($server =~ /^ns(\d+)/) {
		$n = $1;
	} else {
		die "unable to parse server number from name \"$server\"\n";
	}

	if (-e "$testdir/$server/named.notcp") {
		$tcp = "";
	}

	my $ip = "10.53.0.$n";
	if (-e "$testdir/$server/named.ipv6-only") {
		$ip = "fd92:7065:b8e:ffff::$n";
	}

	while (1) {
		my $return = system("$DIG $tcp +noadd +nosea +nostat +noquest +nocomm +nocmd +noedns -p $port version.bind. chaos txt \@$ip > /dev/null");

		last if ($return == 0);

		$tries++;

		if ($tries >= 30) {
			print "I:$test:no response from $server\n";
			print "I:$test:failed\n";

			system("$PERL $topdir/stop.pl $test");

			exit 1;
		}

		sleep 2;
	}
}
