#!/usr/local/bin/perl --					# -*-Perl-*-
eval "exec /usr/local/bin/perl -S $0 $*"
    if 0;

# Copy a Texinfo file, replacing @value's, @FIXME's and other gooddies.

# Copyright © 1996, 2001 Free Software Foundation, Inc.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
# Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program.  If not, write to the Free Software Foundation, Inc.,
# 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

# François Pinard <pinard@iro.umontreal.ca>, 1996.

$_ = <>;
while ($_)
{
    if (/^\@c()$/ || /^\@c (.*)/ || /^\@(include .*)/)
    {
	if ($topseen)
	{
	    print "\@format\n";
	    print "\@strong{\@\@c} $1\n";
	    $_ = <>;
	    while (/\@c (.*)/)
	    {
		print "\@strong{\@\@c} $1\n";
		$_ = <>;
	    }
	    print "\@end format\n";
	}
	else
	{
	    $delay .= "\@format\n";
	    $delay .= "\@strong{\@\@c} $1\n";
	    $_ = <>;
	    while (/\@c (.*)/)
	    {
		$delay .= "\@strong{\@\@c} $1\n";
		$_ = <>;
	    }
	    $delay .= "\@end format\n";
	}
    }
    elsif (/^\@chapter /)
    {
	print;
#	print $delay;
	$delay = '';
	$topseen = 1;
	$_ = <>;
    }
    elsif (/^\@macro /)
    {
	$_ = <> while ($_ && ! /^\@end macro/);
	$_ = <>;
    }
    elsif (/^\@set ([^ ]+) (.*)/)
    {
	$set{$1} = $2;
	$_ = <>;
    }
    elsif (/^\@UNREVISED/)
    {
	print "\@quotation\n";
	print "\@emph{(This message will disappear, once this node is revised.)}\n";
	print "\@end quotation\n";
	$_ = <>;
    }
    else
    {
	while (/\@value{([^\}]*)}/)
	{
	    if (defined $set{$1})
	    {
		$_ = "$`$set{$1}$'";
	    }
	    else
	    {
		$_ = "$`\@strong{<UNDEFINED>}$1\@strong{</UNDEFINED>}$'";
	    }
	}
	while (/\@FIXME-?([a-z]*)\{/)
	{
	    $tag = $1 eq '' ? 'fixme' : $1;
	    $tag =~ y/a-z/A-Z/;
	    print "$`\@strong{<$tag>}";
	    $_ = $';
	    $level = 1;
	    while ($level > 0)
	    {
		if (/([{}])/)
		{
		    if ($1 eq '{')
		    {
			$level++;
			print "$`\{";
			$_ = $';
		    }
		    elsif ($level > 1)
		    {
			$level--;
			print "$`\}";
			$_ = $';
		    }
		    else
		    {
			$level = 0;
			print "$`\@strong{</$tag>}";
			$_ = $';
		    }
		}
		else
		{
		    print;
		    $_ = <>;
		}
	    }
	}
	print;
	$_ = <>;
    }
}
exit 0;

# Local Variables:
# coding: iso-latin-1
# End:
