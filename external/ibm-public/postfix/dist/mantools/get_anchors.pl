#! /usr/bin/perl -w
#
# Copyright (c) 2004 Liviu Daia <Liviu.Daia@imar.ro>
# All rights reserved.
#
# Revision
# Id
# Source
#

use HTML::Parser;

use strict;
use Carp ();
local $SIG{__WARN__} = \&Carp::cluck;

my ($p, $fn, %a);


sub
html_parse_start ($$)
{
  my ($t, $attr) = @_;

  push @{$a{$attr->{name}}}, $fn
    if ($t eq 'a' and defined $attr->{name});
}


$p = HTML::Parser->new(api_version => 3);
$p->strict_comment (0);
$p->report_tags (qw(a));
$p->ignore_elements (qw(script style));

$p->handler (start => \&html_parse_start, 'tagname, attr');

while ($fn = shift)
{
  $p->parse_file ($fn);
  $p->eof;
}

for (keys %a)
{
  print "$_\t\tdefined in ", (join ', ', @{$a{$_}}), "\n"
    if (@{$a{$_}} > 1);
  print "$_\t\tnumerical in ", (join ', ', @{$a{$_}}), "\n"
    if (m/^[\d.]+$/o);
}

