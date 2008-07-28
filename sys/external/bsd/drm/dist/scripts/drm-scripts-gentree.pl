#!/usr/bin/perl
#
# Original version were part of Gerd Knorr's v4l scripts.
#
# Several improvements by (c) 2005-2007 Mauro Carvalho Chehab
#
# Largely re-written (C) 2007 Trent Piepho <xyzzy@speakeasy.org>
# Stolen for DRM usage by airlied
#
# Theory of Operation
#
# This acts as a sort of mini version of cpp, which will process
# #if/#elif/#ifdef/etc directives to strip out code used to support
# multiple kernel versions or otherwise not wanted to be sent upstream to
# git.
#
# Conditional compilation directives fall into two catagories,
# "processed" and "other".  The "other" directives are ignored and simply
# output as they come in without changes (see 'keep' exception).  The
# "processed" variaty are evaluated and only the lines in the 'true' part
# are kept, like cpp would do.
#
# If gentree knows the result of an expression, that directive will be
# "processed", otherwise it will be an "other".  gentree knows the value
# of LINUX_VERSION_CODE, BTTV_VERSION_CODE, the KERNEL_VERSION(x,y,z)
# macro, numeric constants like 0 and 1, and a few defines like MM_KERNEL
# and STV0297_CS2.
#
# An exception is if the comment "/*KEEP*/" appears after the expression,
# in which case that directive will be considered an "other" and not
# processed, other than to remove the keep comment.
#
# Known bugs:
# don't specify the root directory e.g. '/' or even '////'
# directives continued with a back-slash will always be ignored
# you can't modify a source tree in-place, i.e. source dir == dest dir

use strict;
use File::Find;
use Fcntl ':mode';

my $VERSION = shift;
my $SRC = shift;
my $DESTDIR = shift;

if (!defined($DESTDIR)) {
	print "Usage:\ngentree.pl\t<version> <source dir> <dest dir>\n\n";
	exit;
}

my $BTTVCODE = KERNEL_VERSION(0,9,17);
my ($LINUXCODE, $extra) = kernel_version($VERSION);
my $DEBUG = 0;

my %defs = (
	'LINUX_VERSION_CODE' => $LINUXCODE,
	'MM_KERNEL' => ($extra =~ /-mm/)?1:0,
	'DRM_ODD_MM_COMPAT' => 0,
	'I915_HAVE_FENCE' => 1,
	'I915_HAVE_BUFFER' => 1,
	'VIA_HAVE_DMABLIT' => 1,
	'VIA_HAVE_CORE_MM' => 1,
	'VIA_HAVE_FENCE' => 1,
        'VIA_HAVE_BUFFER' => 1,
	'SIS_HAVE_CORE_MM' => 1,
        'DRM_FULL_MM_COMPAT' => 1,   
	'__linux__' => 1,
);

#################################################################
# helpers

sub kernel_version($) {
	$_[0] =~ m/(\d+)\.(\d+)\.(\d+)(.*)/;
	return ($1*65536 + $2*256 + $3, $4);
}

# used in eval()
sub KERNEL_VERSION($$$) { return $_[0]*65536 + $_[1]*256 + $_[2]; }

sub evalexp($) {
	local $_ = shift;
	s|/\*.*?\*/||go;	# delete /* */ comments
	s|//.*$||o;		# delete // comments
	s/\bdefined\s*\(/(/go;	# defined(foo) to (foo)
	while (/\b([_A-Za-z]\w*)\b/go) {
		if (exists $defs{$1}) {
			my $id = $1; my $pos = $-[0];
			s/$id/$defs{$id}/;
			pos = $-[0];
		} elsif ($1 ne 'KERNEL_VERSION') {
			return(undef);
		}
	}
	return(eval($_) ? 1 : 0);
}

#################################################################
# filter out version-specific code

sub filter_source ($$) {
	my ($in,$out) = @_;
	my $line;
	my $level=0;
	my %if = ();
	my %state = ();

	my @dbgargs = \($level, %state, %if, $line);
	sub dbgline($\@) {
		my $level = ${$_[1][0]};
		printf STDERR ("/* BP %4d $_[0] state=$_[1][1]->{$level} if=$_[1][2]->{$level} level=$level (${$_[1][3]}) */\n", $.) if $DEBUG;
	}

	open IN, '<', $in or die "Error opening $in: $!\n";
	open OUT, '>', $out or die "Error opening $out: $!\n";

	print STDERR "File: $in, for kernel $VERSION($LINUXCODE)/\n" if $DEBUG;

	while ($line = <IN>) {
		chomp $line;
		next if ($line =~ m/^#include \"compat.h\"/o);
#		next if ($line =~ m/[\$]Id:/);

		# For "#if 0 /*KEEP*/;" the ; should be dropped too
		if ($line =~ m@^\s*#\s*if(n?def)?\s.*?(\s*/\*\s*(?i)keep\s*\*/;?)@) {
			$state{$level} = "ifother";
			$if{$level} = 1;
			dbgline "#if$1 (keep)", @dbgargs;
			$line =~ s/\Q$2\E//;
			$level++;
		}
		# handle all ifdef/ifndef lines
		elsif ($line =~ /^\s*#\s*if(n?)def\s*(\w+)/o) {
			if (exists $defs{$2}) {
				$state{$level} = 'if';
				$if{$level} = ($1 eq 'n') ? !$defs{$2} : $defs{$2};
				dbgline "#if$1def $2", @dbgargs;
				$level++;
				next;
			}
			$state{$level} = "ifother";
			$if{$level} = 1;
			dbgline "#if$1def (other)", @dbgargs;
			$level++;
		}
		# handle all ifs
		elsif ($line =~ /^\s*#\s*if\s+(.*)$/o) {
			my $res = evalexp($1);
			if (defined $res) {
				$state{$level} = 'if';
				$if{$level} = $res;
				dbgline '#if '.($res?'(yes)':'(no)'), @dbgargs;
				$level++;
				next;
			} else {
				$state{$level} = 'ifother';
				$if{$level} = 1;
				dbgline '#if (other)', @dbgargs;
				$level++;
			}
		}
		# handle all elifs
		elsif ($line =~ /^\s*#\s*elif\s+(.*)$/o) {
			my $exp = $1;
			$level--;
			$level < 0 and die "more elifs than ifs";
			$state{$level} =~ /if/ or die "unmatched elif";

			if ($state{$level} eq 'if' && !$if{$level}) {
				my $res = evalexp($exp);
				defined $res or die 'moving from if to ifother';
				$state{$level} = 'if';
				$if{$level} = $res;
				dbgline '#elif1 '.($res?'(yes)':'(no)'), @dbgargs;
				$level++;
				next;
			} elsif ($state{$level} ne 'ifother') {
				$if{$level} = 0;
				$state{$level} = 'elif';
				dbgline '#elif0', @dbgargs;
				$level++;
				next;
			}
			$level++;
		}
		elsif ($line =~ /^\s*#\s*else/o) {
			$level--;
			$level < 0 and die "more elses than ifs";
			$state{$level} =~ /if/ or die "unmatched else";
			$if{$level} = !$if{$level} if ($state{$level} eq 'if');
			$state{$level} =~ s/^if/else/o; # if -> else, ifother -> elseother, elif -> elif
			dbgline '#else', @dbgargs;
			$level++;
			next if $state{$level-1} !~ /other$/o;
		}
		elsif ($line =~ /^\s*#\s*endif/o) {
			$level--;
			$level < 0 and die "more endifs than ifs";
			dbgline '#endif', @dbgargs;
			next if $state{$level} !~ /other$/o;
		}

		my $print = 1;
		for (my $i=0;$i<$level;$i++) {
			next if $state{$i} =~ /other$/o;	# keep code in ifother/elseother blocks
			if (!$if{$i}) {
				$print = 0;
				dbgline 'DEL', @{[\$i, \%state, \%if, \$line]};
				last;
			}
		}
		print OUT "$line\n" if $print;
	}
	close IN;
	close OUT;
}

#################################################################

sub parse_dir {
	my $file = $File::Find::name;

	return if ($file =~ /CVS/);
	return if ($file =~ /~$/);

	my $f2 = $file;
	$f2 =~ s/^\Q$SRC\E/$DESTDIR/;

	my $mode = (stat($file))[2];
	if ($mode & S_IFDIR) {
		print("mkdir -p '$f2'\n");
		system("mkdir -p '$f2'");  # should check for error
		return;
	}
	print "from $file to $f2\n";

	if ($file =~ m/.*\.[ch]$/) {
		filter_source($file, $f2);
	} else {
		system("cp $file $f2");
	}
}


# main

printf "kernel is %s (0x%x)\n",$VERSION,$LINUXCODE;

# remove any trailing slashes from dir names.  don't pass in just '/'
$SRC =~ s|/*$||; $DESTDIR =~ s|/*$||;

print "finding files at $SRC\n";

find({wanted => \&parse_dir, no_chdir => 1}, $SRC);
