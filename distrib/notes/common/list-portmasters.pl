#!/usr/bin/env perl

#
# Usage:
# 1) lynx -dump http://www.netbsd.org/People/port-maintainers.html \
#    | perl list-portmasters.pl
#    >x
# 2) Last entry will need fixing (BUG)
# 3) Sort: sort +1 x >xx ; mv xx x
# 4) merge "x" into src/distrib/notes/common/main's "portmasters" section
#

while(<>) {
    chomp;
    last if /Port.*Name.*E-mail.*Address/;
}

while(<>) {
    chomp;
    s/^\s*//;
    s/\s*$//;
    @a = split(/\s+/);

    $port=shift(@a);
    $email=pop(@a);
    $name="@a";

    if ($port !~ /\[\d+\]/) {
	# fixup needed *sigh*
	$last_name .= " $last_email";
	$last_email = $email;

	$name = "$port $name";
	$port = $last_port;
	$email = <>;
	chomp($email);
    }

    # valid data is now in $last_*,
    # $* is saved for possible further processing
    
    $last_port=~s,\s*\[\d+\],,;
    $last_name=~s,\s*\[\d+\],,;
    $last_email=~s,\s*\[\d+\],,;

    $last_name=~s,ø,\(/o,;		# Søren => S\(/oren
    
    # output
    print ".It ";
    printf("Ta %-30s", $last_name);
    print " ";
    printf("Ta Mt %20s", $last_email);
    print " ";
    printf("Ta Sy %s", $last_port);
    print "\n";

    $last_port=$port;
    $last_email=$email;
    $last_name=$name;

    last if $name=~/______________/;
}
