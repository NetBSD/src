#!/usr/bin/perl

# mengwong@pobox.com
# Wed Dec 10 03:52:04 EST 2003
# postfix-policyd-spf
# version 1.03
# see http://spf.pobox.com/

use Fcntl;
use Sys::Syslog qw(:DEFAULT setlogsock);
use strict;

# ----------------------------------------------------------
# 		       configuration
# ----------------------------------------------------------

# to use SPF, install Mail::SPF::Query from CPAN or from the SPF website at http://spf.pobox.com/downloads.html
# then uncomment the SPF line.

  my @HANDLERS;
  push @HANDLERS, "testing";
# push @HANDLERS, "sender_permitted_from"; use Mail::SPF::Query;

my $VERBOSE = 1;

my $DEFAULT_RESPONSE = "DUNNO";

#
# Syslogging options for verbose mode and for fatal errors.
# NOTE: comment out the $syslog_socktype line if syslogging does not
# work on your system.
#

my $syslog_socktype = 'unix'; # inet, unix, stream, console
my $syslog_facility = "mail";
my $syslog_options  = "pid";
my $syslog_priority = "info";
my $syslog_ident    = "postfix/policy-spf";

# ----------------------------------------------------------
#		   minimal documentation
# ----------------------------------------------------------

#
# Usage: smtpd-policy.pl [-v]
#
# Demo delegated Postfix SMTPD policy server.
# This server implements SPF.
# Another server implements greylisting.
# Postfix has a pluggable policy server architecture.
# You can call one or both from Postfix.
# 
# The SPF handler uses Mail::SPF::Query to do the heavy lifting.
# 
# This documentation assumes you have read Postfix's README_FILES/SMTPD_POLICY_README
# 
# Logging is sent to syslogd.
#
# How it works: each time a Postfix SMTP server process is started
# it connects to the policy service socket, and Postfix runs one
# instance of this PERL script.  By default, a Postfix SMTP server
# process terminates after 100 seconds of idle time, or after serving
# 100 clients. Thus, the cost of starting this PERL script is smoothed
# out over time.
#
# To run this from /etc/postfix/master.cf:
#
#    policy  unix  -       n       n       -       -       spawn
#      user=nobody argv=/usr/bin/perl /usr/libexec/postfix/smtpd-policy.pl
#
# To use this from Postfix SMTPD, use in /etc/postfix/main.cf:
#
#    smtpd_recipient_restrictions =
#	...
#	reject_unauth_destination
#	check_policy_service unix:private/policy
#	...
#
# NOTE: specify check_policy_service AFTER reject_unauth_destination
# or else your system can become an open relay.
#
# To test this script by hand, execute:
#
#    % perl smtpd-policy.pl
#
# Each query is a bunch of attributes. Order does not matter, and
# the demo script uses only a few of all the attributes shown below:
#
#    request=smtpd_access_policy
#    protocol_state=RCPT
#    protocol_name=SMTP
#    helo_name=some.domain.tld
#    queue_id=8045F2AB23
#    sender=foo@bar.tld
#    recipient=bar@foo.tld
#    client_address=1.2.3.4
#    client_name=another.domain.tld
#    [empty line]
#
# The policy server script will answer in the same style, with an
# attribute list followed by a empty line:
#
#    action=dunno
#    [empty line]
#

# Jul 23 18:43:29 dumbo/dumbo policyd[21171]: Attribute: client_address=208.210.125.227
# Jul 23 18:43:29 dumbo/dumbo policyd[21171]: Attribute: client_name=newbabe.mengwong.com
# Jul 23 18:43:29 dumbo/dumbo policyd[21171]: Attribute: helo_name=newbabe.mengwong.com
# Jul 23 18:43:29 dumbo/dumbo policyd[21171]: Attribute: protocol_name=ESMTP
# Jul 23 18:43:29 dumbo/dumbo policyd[21171]: Attribute: protocol_state=RCPT
# Jul 23 18:43:29 dumbo/dumbo policyd[21171]: Attribute: queue_id=
# Jul 23 18:43:29 dumbo/dumbo policyd[21171]: Attribute: recipient=mengwong@dumbo.pobox.com
# Jul 23 18:43:29 dumbo/dumbo policyd[21171]: Attribute: request=smtpd_access_policy
# Jul 23 18:43:29 dumbo/dumbo policyd[21171]: Attribute: sender=mengwong@newbabe.mengwong.com

# ----------------------------------------------------------
# 		       initialization
# ----------------------------------------------------------

#
# Log an error and abort.
#
sub fatal_exit {
  syslog(err  => "fatal_exit: @_");
  syslog(warn => "fatal_exit: @_");
  syslog(info => "fatal_exit: @_");
  die "fatal: @_";
}

#
# Unbuffer standard output.
#
select((select(STDOUT), $| = 1)[0]);

#
# This process runs as a daemon, so it can't log to a terminal. Use
# syslog so that people can actually see our messages.
#
setlogsock $syslog_socktype;
openlog $syslog_ident, $syslog_options, $syslog_facility;

# ----------------------------------------------------------
# 			    main
# ----------------------------------------------------------

#
# Receive a bunch of attributes, evaluate the policy, send the result.
#
my %attr;
while (<STDIN>) {
  chomp;
  if (/=/)       { my ($k, $v) = split (/=/, $_, 2); $attr{$k} = $v; next }
  elsif (length) { syslog(warn=>sprintf("warning: ignoring garbage: %.100s", $_)); next; }

  if ($VERBOSE) {
    for (sort keys %attr) {
      syslog(debug=> "Attribute: %s=%s", $_, $attr{$_});
    }
  }

  fatal_exit ("unrecognized request type: '$attr{request}'") unless $attr{request} eq "smtpd_access_policy";

  my $action = $DEFAULT_RESPONSE;
  my %responses;
  foreach my $handler (@HANDLERS) {
    no strict 'refs';
    my $response = $handler->(attr=>\%attr);
    syslog(debug=> "handler %s: %s", $handler, $response);
    if ($response and $response !~ /^dunno/i) {
      syslog(info=> "handler %s: %s is decisive.", $handler, $response);
      $action = $response; last;
    }
  }

  syslog(info=> "decided action=%s", $action);

  print STDOUT "action=$action\n\n";
  %attr = ();
}

# ----------------------------------------------------------
# 		      plugin: SPF
# ----------------------------------------------------------
sub sender_permitted_from {
  local %_ = @_;
  my %attr = %{ $_{attr} };

  my $query = new Mail::SPF::Query (ip    =>$attr{client_address},
				    sender=>$attr{sender},
				    helo  =>$attr{helo_name});
  my ($result, $smtp_comment, $header_comment) = $query->result();

  syslog(info=>"%s: SPF %s: smtp_comment=%s, header_comment=%s",
	 $attr{queue_id}, $result, $smtp_comment, $header_comment); 

  if    ($result eq "pass")  { return "DUNNO"; }
  elsif ($result eq "fail")  { return "REJECT " . ($smtp_comment || $header_comment); }
  elsif ($result eq "error") { return "450 temporary failure: $smtp_comemnt"; }
  else                       { return "DUNNO"; }
  # unknown, softfail, and none all return DUNNO

  # TODO XXX: prepend Received-SPF header.  Wietse says he will add that functionality soon.
}

# ----------------------------------------------------------
# 		      plugin: testing
# ----------------------------------------------------------
sub testing {
  local %_ = @_;
  my %attr = %{ $_{attr} };

  if (lc address_stripped($attr{sender}) eq
      lc address_stripped($attr{recipient})
      and
      $attr{recipient} =~ /policyblock/) {

    syslog(info=>"%s: testing: will block as requested",
	   $attr{queue_id}); 
    return "REJECT smtpd-policy blocking $attr{recipient}";
  }
  else {
    syslog(info=>"%s: testing: stripped sender=%s, stripped rcpt=%s",
	   $attr{queue_id},
	   address_stripped($attr{sender}),
	   address_stripped($attr{recipient}),
	   ); 
    
  }
  return "DUNNO";
}

sub address_stripped {
  # my $foo = localpart_lhs('foo+bar@baz.com'); # returns 'foo@baz.com'
  my $string = shift;
  for ($string) {
    s/[+-].*\@/\@/;
  }
  return $string;
}

