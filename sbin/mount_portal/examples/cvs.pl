#!/usr/pkg/bin/perl

# ensure that HOME is set for pserver modes
$ENV{'HOME'} = (getpwuid($<))[7] || "/";
# supplement path to include ssh and cvs
$ENV{'PATH'} = "/usr/pkg/bin:$ENV{'PATH'}";
# use ssh instead of rsh for cvs connections
$ENV{'CVS_RSH'} = "ssh";

if    ($ARGV[0] eq "netbsd") {
    $ENV{'CVSROOT'} = "anoncvs\@anoncvs.netbsd.org:/cvsroot";
    shift(@ARGV);
}
elsif ($ARGV[0] eq "freebsd") {
    $ENV{'CVSROOT'} = ":pserver:anoncvs\@anoncvs.freebsd.org:/home/ncvs";
    shift(@ARGV);
}
elsif ($ARGV[0] eq "openbsd") {
    $ENV{'CVSROOT'} = "anoncvs\@anoncvs.usa.openbsd.org:/cvs";
    shift(@ARGV);
}
else {
    print("configuration not supported\n");
    exit(0);
}

foreach $file (@ARGV) {
    ($file, $rev) = ($1,$3) if ($file =~ /(.*),(-r)?([\d\.]+|[-a-zA-Z\d]+)$/);
    $cmd = "co -p";
    $rev = "-r $rev" if ($rev);
    system("cvs $cmd $rev $file");
}
