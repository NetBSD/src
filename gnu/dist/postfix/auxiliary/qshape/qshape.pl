#! /usr/bin/perl -w

# To view the formatted manual page of this file, type:
#	POSTFIXSOURCE/mantools/srctoman - qshape | nroff -man

#++
# NAME
#	qshape 1
# SUMMARY
#	Print Postfix queue domain and age distribution
# SYNOPSIS
# .fi
#	\fBqshape\fR [\fB-s\fR] [\fB-p\fR] [\fB-m \fImin_subdomains\fR]
#		[\fB-b \fIbucket_count\fR] [\fB-t \fIbucket_time\fR]
#		[\fB-l\fR] [\fB-w \fIterminal_width\fR]
#		[\fB-N \fIbatch_msg_count\fR] [\fB-n \fIbatch_top_domains\fR]
#		[\fB-c \fIconfig_directory\fR] [\fIqueue_name\fR ...]
# DESCRIPTION
#	The \fBqshape\fR program helps the administrator understand the
#	Postfix queue message distribution in time and by sender domain
#	or recipient domain. The program needs read access to the queue
#	directories and queue files, so it must run as the superuser or
#	the \fBmail_owner\fR specified in \fBmain.cf\fR (typically
#	\fBpostfix\fR).
#
#	Options:
# .IP \fB-s\fR
#	Display the sender domain distribution instead of the recipient
#	domain distribution.  By default the recipient distribution is
#	displayed. There can be more recipients than messages, but as
#	each message has only one sender, the sender distribution is a
#	message distribution.
# .IP \fB-p\fR
#	Generate aggregate statistics for parent domains. Top level domains
#	are not shown, nor are domains with fewer than \fImin_subdomains\fR
#	subdomains. The names of parent domains are shown with a leading dot,
#	(e.g. \fI.example.com\fR).
# .IP "\fB-m \fImin_subdomains\fR"
#	When used with the \fB-p\fR option, sets the minimum subdomain count
#	needed to show a separate line for a parent domain. The default is 5.
# .IP "\fB-b \fIbucket_count\fR"
#	The age distribution is broken up into a sequence of geometrically
#	increasing intervals. This option sets the number of intervals
#	or "buckets". Each bucket has a maximum queue age that is twice
#	as large as that of the previous bucket. The last bucket has no
#	age limit.
# .IP "\fB-t \fIbucket_time\fR"
#	The age limit in minutes for the first time bucket. The default
#	value is 5, meaning that the first bucket counts messages between
#	0 and 5 minutes old.
# .IP "\fB-l\fR"
#	Instead of using a geometric age sequence, use a linear age sequence,
#	in other words simple multiples of \fBbucket_time\fR.
#
#	This feature is available in Postfix 2.2 and later.
# .IP "\fB-w \fIterminal_width\fR"
#	The output is right justified, with the counts for the last
#	bucket shown on the 80th column, the \fIterminal_width\fR can be
#	adjusted for wider screens allowing more buckets to be displayed
#	without truncating the domain names on the left. When a row for a
#	full domain name and its counters does not fit in the specified
#	number of columns, only the last 17 bytes of the domain name
#	are shown with the prefix replaced by a '+' character. Truncated
#	parent domain rows are shown as '.+' followed by the last 16 bytes
#	of the domain name. If this is still too narrow to show the domain
#	name and all the counters, the terminal_width limit is violated.
# .IP "\fB-N \fIbatch_msg_count\fR"
#	When the output device is a terminal, intermediate results are
#	shown each "batch_msg_count" messages. This produces usable results
#	in a reasonable time even when the deferred queue is large. The
#	default is to show intermediate results every 1000 messages.
# .IP "\fB-n \fIbatch_top_domains\fR"
#	When reporting intermediate or final results to a termainal, report
#	only the top "batch_top_domains" domains. The default limit is 20
#	domains.
# .IP "\fB-c \fIconfig_directory\fR"
#	The \fBmain.cf\fR configuration file is in the named directory
#	instead of the default configuration directory.
# .PP
#	Arguments:
# .IP \fIqueue_name\fR
#	By default \fBqshape\fR displays the combined distribution of
#	the incoming and active queues. To display a different set of
#	queues, just list their directory names on the command line.
#	Absolute paths are used as is, other paths are taken relative
#	to the \fBmain.cf\fR \fBqueue_directory\fR parameter setting.
#	While \fBmain.cf\fR supports the use of \fI$variable\fR expansion
#	in the definition of the \fBqueue_directory\fR parameter, the
#	\fBqshape\fR program does not. If you must use variable expansions
#	in the \fBqueue_directory\fR setting, you must specify an explicit
#	absolute path for each queue subdirectory even if you want the
#	default incoming and active queue distribution.
# SEE ALSO
#	mailq(1), List all messages in the queue.
#	QSHAPE_README Examples and background material.
# FILES
#	$config_directory/main.cf, Postfix installation parameters.
#	$queue_directory/maildrop/, local submission directory.
#	$queue_directory/incoming/, new message queue.
#	$queue_directory/hold/, messages waiting for tech support.
#	$queue_directory/active/, messages scheduled for delivery.
#	$queue_directory/deferred/, messages postponed for later delivery.
# LICENSE
# .ad
# .fi
#	The Secure Mailer license must be distributed with this software.
# AUTHOR(S)
#	Victor Duchovni
#	Morgan Stanley
#--

use strict;
use IO::File;
use File::Find;
use Getopt::Std;

my $cls;		# Clear screen escape sequence
my $batch_msg_count;	# Interim result frequency
my $batch_top_domains;	# Interim result count
my %opts;		# Command line switches
my %q;			# domain counts for queues and buckets
my %sub;		# subdomain counts for parent domains
my $now = time;		# reference time
my $bnum = 10;		# deferred queue bucket count
my $width = 80;		# screen char width
my $dwidth = 18;	# min width of domain field
my $tick = 5; 		# minutes
my $minsub = 5;		# Show parent domains with at least $minsub subdomains
my @qlist = qw(incoming active);

do {
    local $SIG{__WARN__} = sub {
    	warn "$0: $_[0]" unless exists($opts{"h"});
	die "Usage: $0 [ -s ] [ -p ] [ -m <min_subdomains> ] [ -l ]\n".
	    "\t[ -b <bucket_count> ] [ -t <bucket_time> ] [ -w <terminal_width> ]\n".
	    "\t[ -N <batch_msg_count> ] [ -n <batch_top_domains> ]\n".
	    "\t[ -c <config_directory> ] [ <queue_name> ... ]\n".
	"The 's' option shows sender domain counts.\n".
	"The 'p' option shows address counts by for parent domains.\n".
	"Parent domains are shown with a leading '.' before the domain name.\n".
	"Parent domains are only shown if the the domain is not a TLD, and at\n".
	"least <min_subdomains> (default 5) subdomains are shown in the output.\n\n".

	"The bucket age ranges in units of <bucket_time> minutes are\n".
	"[0,1), [1,2), [2,4), [4,8), [8, 16), ... i.e.:\n".
	"\tthe first bucket is [0, bucket_time) minutes\n".
	"\tthe second bucket is [bucket_time, 2*bucket_time) minutes\n".
	"\tthe third bucket is [2*bucket_time, 4*bucket_time) minutes...\n".
	"'-l' makes the ages linear, the number of buckets shown is <bucket_count>\n\n".

	"The default summary is for the incoming and active queues. An explicit\n".
	"list of queue names can be given on the command line. Non-absolute queue\n".
	"names are interpreted relative to the Postfix queue directory. Use\n".
	"<config_directory> to specify a non-default Postfix instance.  Values of\n".
	"the main.cf queue_directory parameter that use variable expansions are\n".
	"not supported. If necessary, use explicit absolute paths for all queues.\n";
    };

    getopts("lhc:psw:b:t:m:n:N:", \%opts);
    warn "Help message" if (exists $opts{"h"});

    @qlist = @ARGV if (@ARGV > 0);

    # The -c option specifies the configuration directory,
    # it is not used if all queue names are absolute.
    #
    foreach (@qlist) {
	next if (m{^/});

	$ENV{q{MAIL_CONFIG}} = $opts{"c"} if (exists $opts{"c"});

	chomp(my $qdir = qx{postconf -h queue_directory});
	die "$0: postconf failed\n" if ($? != 0);
	warn "'queue_directory' variable expansion not supported: $qdir\n"
	    if ($qdir =~ /\$/);
	chdir($qdir) or die "$0: chdir($qdir): $!\n";
	last;
    }
};

$width = $opts{"w"} if (exists $opts{"w"} && $opts{"w"} > 80);
$bnum = $opts{"b"} if (exists $opts{"b"} && $opts{"b"} > 0);
$tick = $opts{"t"} if (exists $opts{"t"} && $opts{"t"} > 0);
$minsub = $opts{"m"} if (exists $opts{"m"} && $opts{"m"} > 0);

if ( -t STDOUT ) {
    $batch_msg_count = 1000 unless defined($batch_msg_count = $opts{"N"});
    $batch_top_domains = 20 unless defined ($batch_top_domains = $opts{"n"});
    $cls = `clear`;
} else {
    $batch_msg_count = 0;
    $batch_top_domains = 0;
    $cls = "";
}

sub rec_get {
    my ($h) = @_;
    my $r = getc($h) || return;
    my $l = 0;
    my $shift = 0;
    while (defined(my $lb = getc($h))) {
	my $o = ord($lb);
	$l |= ($o & 0x7f) << $shift ;
	last if (($o & 0x80) == 0);
	$shift += 7;
	return if ($shift > 14); # XXX: max rec len of 2097151
    }
    my $d = "";
    return unless ($l == 0 || read($h,$d,$l) == $l);
    ($r, $l, $d);
}

sub qenv {
    my ($qfile) = @_;
    return unless $qfile =~ m{(^|/)[A-F0-9]{6,}$};
    my @st = lstat($qfile);
    return unless (@st > 0 && -f _ && (($st[2] & 0733) == 0700));

    my $h = new IO::File($qfile, "r") || return;
    my ($t, $s, @r, $dlen);
    my ($r, $l, $d) = rec_get($h);

    if ($r eq "C") {
	# XXX: Sanity check, the first record type is REC_TYPE_SIZE (C)
	# if the file is proper queue file written by "cleanup", in
	# this case the second record is always REC_TYPE_TIME.
	#
	$dlen = $1 if ($d =~ /^\s*(\d+)\s+\d+\s+\d+/);
	($r, $l, $d) = rec_get($h);
	return unless (defined $r && $r eq "T");
	($t) = split(/\s+/, $d);
    } elsif ($r eq "S" || $r eq "F") {
	# For embryonic queue files in the "maildrop" directory the first
	# record is either a REC_TYPE_FULL (F) followed by REC_TYPE_FROM
	# or an immediate REC_TYPE_FROM (S). In either case there is no
	# REC_TYPE_TIME and we get the timestamp via lstat().
	#
	$t = $st[9];
    	if ($r ne "S") {
	    ($r, $l, $d) = rec_get($h);
	    return unless (defined $r && $r eq "S");
	}
	$s = $d;
    } else {
    	# XXX: Not a valid queue file!
	#
	return undef;
    }
    while (my ($r, $l, $d) = rec_get($h)) {
	if ($r eq "p" && $d > 0) {
	    seek($h, $d, 0) or return (); # follow pointer
	}
	if ($r eq "R") { push(@r, $d); }
	elsif ($r eq "S") { $s = $d; }
	elsif ($r eq "M") {
	    last unless (defined($s));
	    if (defined($dlen)) {
		seek($h, $dlen, 1) or return (); # skip content
		($r, $l, $d) = rec_get($h);
	    } else {
		while ((($r, $l, $d) = rec_get($h)) && ($r =~ /^[NLp]$/)) {
		    if ($r eq "p" && $d > 0) {
			seek($h, $d, 0) or return (); # follow pointer
		    }
		}
	    }
	    return unless (defined($r) && $r eq "X");
	}
	elsif ($r eq "E") {
	    last unless (defined($t) && defined($s) && @r);
	    return ($t, $s, @r);
	}
    }
    return ();
}

# bucket 0 is the total over all the buckets.
# buckets 1 to $bnum contain the age breakdown.
#
sub bucket {
    my ($qt, $now) = @_;
    my $m = ($now - $qt) / (60 * $tick);
    return 1 if ($m < 1);
    my $b = $opts{"l"} ? int($m+1) : 2 + int(log($m) / log(2));
    $b < $bnum ? $b : $bnum;
}

# Collate by age of message in the selected queues.
#
my $msgs;
sub wanted {
    if (my ($t, $s, @r) = qenv($_)) {
	my $b = bucket($t, $now);
	foreach my $a (map {lc($_)} ($opts{"s"} ? ($s) : @r)) {
	    ++$q{"TOTAL"}->[0];
	    ++$q{"TOTAL"}->[$b];
	    $a = "MAILER-DAEMON" if ($a eq "");
	    $a =~ s/.*\@//;
	    $a =~ s/\.\././g;
	    $a =~ s/\.?(.+?)\.?$/$1/;
	    my $new = 0;
	    do {
		my $old = (++$q{$a}->[0] > 1);
		++$q{$a}->[$b];
		++$sub{$a} if ($new);
		$new = ! $old;
	    } while ($opts{"p"} && $a =~ s/^(?:\.)?[^.]+\.(.*\.)/.$1/);
	}
    	if ($batch_msg_count > 0 && ++$msgs % $batch_msg_count == 0) {
	    results();
	}
    }
}

my @heads;
my $fmt;
my $dw;

sub pdomain {
    my ($d, @count) = @_;
    foreach ((0 .. $bnum)) { $count[$_] ||= 0; }
    my $len = length($d);
    if ($len > $dw) {
	if (substr($d, 0, 1) eq ".") {
	    print ".+",substr($d, $len-$dw+2, $dw-2);
	} else {
	    print "+",substr($d, $len-$dw+1, $dw-1);
	}
    } else {
    	print (" " x ($dw - $len), $d);
    }
    printf "$fmt\n", @count;
}

sub results {
    @heads = ();
    $dw = $width;
    $fmt = "";
    for (my $i = 0, my $t = 0; $i <= $bnum; ) {
	$q{"TOTAL"}->[$i] ||= 0;
	my $l = length($q{"TOTAL"}->[$i]);
	my $h = ($i == 0) ? "T" : $t;
	$l = length($h) if (length($h) >= $l);
	$l = ($l > 2) ? $l + 1 : 3;
	push(@heads, $h);
	$fmt .= sprintf "%%%ds", $l;
	$dw -= $l;
	if (++$i < $bnum) { $t += ($t && !$opts{"l"}) ? $t : $tick; } else { $t = "$t+"; }
    }
    $dw = $dwidth if ($dw < $dwidth);

    print $cls if ($batch_msg_count > 0);

    # Print headings
    #
    pdomain("", @heads);

    my $n = 0;

    # Show per-domain totals
    #
    foreach my $d (sort { $q{$b}->[0] <=> $q{$a}->[0] ||
			   length($a) <=> length($b) } keys %q) {

	# Skip parent domains with < $minsub subdomains.
	#
	next if ($d =~ /^\./ && $sub{$d} < $minsub);

	last if ($batch_top_domains > 0 && ++$n > $batch_top_domains);

	pdomain($d, @{$q{$d}});
    }
}

find(\&wanted, @qlist);
results();
