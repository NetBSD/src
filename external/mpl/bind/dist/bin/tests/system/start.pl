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

# Framework for starting test servers.
# Based on the type of server specified, check for port availability, remove
# temporary files, start the server, and verify that the server is running.
# If a server is specified, start it. Otherwise, start all servers for test.

use strict;
use Cwd;
use Cwd 'abs_path';
use Getopt::Long;

# Usage:
#   perl start.pl [--noclean] [--restart] [--port port] test [server [options]]
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

my $usage = "usage: $0 [--noclean] [--restart] [--port <port>] test-directory [server-directory [server-options]]";
my $noclean = '';
my $restart = '';
my $queryport = 5300;

GetOptions('noclean' => \$noclean, 'restart' => \$restart, 'port=i' => \$queryport) or die "$usage\n";

my $test = $ARGV[0];
my $server = $ARGV[1];
my $options = $ARGV[2];

if (!$test) {
	die "$usage\n";
}
if (!-d $test) {
	die "No test directory: \"$test\"\n";
}
if ($server && !-d "$test/$server") {
	die "No server directory: \"$test/$server\"\n";
}

# Global variables
my $topdir = abs_path("$test/..");
my $testdir = abs_path("$test");
my $NAMED = $ENV{'NAMED'};
my $DIG = $ENV{'DIG'};
my $PERL = $ENV{'PERL'};
my $PYTHON = $ENV{'PYTHON'};

# Start the server(s)

if ($server) {
	if ($server =~ /^ns/) {
		&check_ports($server);
	}
	&start_server($server, $options);
	if ($server =~ /^ns/) {
		&verify_server($server);
	}
} else {
	# Determine which servers need to be started for this test.
	opendir DIR, $testdir;
	my @files = sort readdir DIR;
	closedir DIR;

	my @ns = grep /^ns[0-9]*$/, @files;
	my @ans = grep /^ans[0-9]*$/, @files;
	my $name;

	# Start the servers we found.
	&check_ports();
	foreach $name(@ns, @ans) {
		&start_server($name);
		&verify_server($name) if ($name =~ /^ns/);
	}
}

# Subroutines

sub check_ports {
	my $server = shift;
	my $options = "";
	my $port = $queryport;
	my $file = "";

	$file = $testdir . "/" . $server . "/named.port" if ($server);

	if ($server && $server =~ /(\d+)$/) {
		$options = "-i $1";
	}

	if ($file ne "" && -e $file) {
		open(FH, "<", $file);
		while(my $line=<FH>) {
			chomp $line;
			$port = $line;
			last;
		}
		close FH;
	}

	my $tries = 0;
	while (1) {
		my $return = system("$PERL $topdir/testsock.pl -p $port $options");
		last if ($return == 0);
		if (++$tries > 4) {
			print "$0: could not bind to server addresses, still running?\n";
			print "I:server sockets not available\n";
			print "I:failed\n";
			system("$PERL $topdir/stop.pl $testdir"); # Is this the correct behavior?
			exit 1;
		}
		print "I:Couldn't bind to socket (yet)\n";
		sleep 2;
	}
}

sub start_server {
	my $server = shift;
	my $options = shift;

	my $cleanup_files;
	my $command;
	my $pid_file;
        my $cwd = getcwd();
	my $args_file = $cwd . "/" . $test . "/" . $server . "/" . "named.args";

	if ($server =~ /^ns/) {
		$cleanup_files = "{*.jnl,*.bk,*.st,named.run}";
		if ($ENV{'USE_VALGRIND'}) {
			$command = "valgrind -q --gen-suppressions=all --num-callers=48 --fullpath-after= --log-file=named-$server-valgrind-%p.log ";
			if ($ENV{'USE_VALGRIND'} eq 'helgrind') {
				$command .= "--tool=helgrind ";
			} else {
				$command .= "--tool=memcheck --track-origins=yes --leak-check=full ";
			}
			$command .= "$NAMED -m none -M external ";
		} else {
			$command = "$NAMED ";
		}
		if ($options) {
			$command .= "$options";
		} elsif (-e $args_file) {
			open(FH, "<", $args_file);
			while(my $line=<FH>)
			{
				#$line =~ s/\R//g;
				chomp $line;
				next if ($line =~ /^\s*$/); #discard blank lines
				next if ($line =~ /^\s*#/); #discard comment lines
				$line =~ s/#.*$//g;
				$options = $line;
				last;
			}
			close FH;
			$command .= "$options";
		} else {
			$command .= "-D $test-$server ";
			$command .= "-X named.lock ";
			$command .= "-m record,size,mctx ";
			$command .= "-T clienttest ";
			$command .= "-T nosoa "
				if (-e "$testdir/$server/named.nosoa");
			$command .= "-T noaa "
				if (-e "$testdir/$server/named.noaa");
			$command .= "-T noedns "
				if (-e "$testdir/$server/named.noedns");
			$command .= "-T dropedns "
				if (-e "$testdir/$server/named.dropedns");
			$command .= "-T maxudp512 "
				if (-e "$testdir/$server/named.maxudp512");
			$command .= "-T maxudp1460 "
				if (-e "$testdir/$server/named.maxudp1460");
			$command .= "-c named.conf -d 99 -g -U 4";
		}
		$command .= " -T notcp"
			if (-e "$testdir/$server/named.notcp");
		if ($restart) {
			$command .= " >>named.run 2>&1 &";
		} else {
			$command .= " >named.run 2>&1 &";
		}
		$pid_file = "named.pid";
	} elsif ($server =~ /^ans/) {
		$cleanup_files = "{ans.run}";
                if (-e "$testdir/$server/ans.py") {
                        $command = "$PYTHON -u ans.py 10.53.0.$' $queryport";
                } elsif (-e "$testdir/$server/ans.pl") {
                        $command = "$PERL ans.pl";
                } else {
                        $command = "$PERL $topdir/ans.pl 10.53.0.$'";
                }
		if ($options) {
			$command .= "$options";
		} else {
			$command .= "";
		}
		if ($restart) {
			$command .= " >>ans.run 2>&1 &";
		} else {
			$command .= " >ans.run 2>&1 &";
		}
		$pid_file = "ans.pid";
	} else {
		print "I:Unknown server type $server\n";
		print "I:failed\n";
		system "$PERL $topdir/stop.pl $testdir";
		exit 1;
	}

	# print "I:starting server %s\n",$server;

	chdir "$testdir/$server";

	unless ($noclean) {
		unlink glob $cleanup_files;
	}

	# get the shell to report the pid of the server ($!)
	$command .= "echo \$!";

	# start the server
	my $child = `$command`;
	$child =~ s/\s+$//g;

	# wait up to 14 seconds for the server to start and to write the
	# pid file otherwise kill this server and any others that have
	# already been started
	my $tries = 0;
	while (!-s $pid_file) {
		if (++$tries > 140) {
			print "I:Couldn't start server $server (pid=$child)\n";
			print "I:failed\n";
			system "kill -9 $child" if ("$child" ne "");
			system "$PERL $topdir/stop.pl $testdir";
			exit 1;
		}
		# sleep for 0.1 seconds
		select undef,undef,undef,0.1;
	}

        # go back to the top level directory
	chdir $cwd;
}

sub verify_server {
	my $server = shift;
	my $n = $server;
	my $port = $queryport;
	my $tcp = "+tcp";

	$n =~ s/^ns//;

	if (-e "$testdir/$server/named.port") {
		open(FH, "<", "$testdir/$server/named.port");
		while(my $line=<FH>) {
			chomp $line;
			$port = $line;
			last;
		}
		close FH;
	}

	$tcp = "" if (-e "$testdir/$server/named.notcp");

	my $tries = 0;
	while (1) {
		my $return = system("$DIG $tcp +noadd +nosea +nostat +noquest +nocomm +nocmd +noedns -p $port version.bind. chaos txt \@10.53.0.$n > dig.out");
		last if ($return == 0);
		if (++$tries >= 30) {
			print "I:no response from $server\n";
			print "I:failed\n";
			system("$PERL $topdir/stop.pl $testdir");
			exit 1;
		}
		sleep 2;
	}
	unlink "dig.out";
}
