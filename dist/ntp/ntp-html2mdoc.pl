#!/usr/pkg/bin/perl

@longmonth = ( "January", "February", "March", "April", "May", "June", "July",
	       "August", "September", "October", "November", "December" );
# require "ctime.pl";

$sec = 8;				# XXX

sub swallow {
	my ($what) = @_;

	while (<>) {
		chomp;
		return if /$what/i;
	}
}

sub chew {
	my ($match, $reset) = @_;

	$_ = undef if $reset;
	while (!(/$match/i)) {
		chomp($_ .= " " . <>);
	}
}

sub detag {
	s/<[^>]+>//g;
}

sub dehtmlchar {
	s/&nbsp;/ /g;
	s/&lt;/</g;
	s/&gt;/>/g;
}

($mday, $mon, $year) = (localtime(time))[3 .. 5];
$date = sprintf "%s %02d, %d", $longmonth[$mon], $mday, $year + 1900;

swallow("<H3>");
chomp($_ = <>);
detag;
($name, $descr) = split(' ', $_, 2);
$descr =~ s/^\s*-\s*//;
($NAME = $name) =~ tr/a-z/A-Z/;

print <<EOF;
.\\"	\$NetBSD\$
.\\" Converted from HTML to mandoc by ntp-html2mdoc.pl
.\\"
.Dd $date
.Dt $NAME $sec
.Os
.Sh NAME
.Nm $name
.Nd $descr
.Sh SYNOPSIS
.Nm
EOF

swallow("Synopsis");
chew("</TT>", 1);
detag;
s/^\s*$name\s*//;
@args = split;
while ($_ = shift @args) {
	next if /\[/;
	s/^-//;
	print ".Op Fl $_";
	if (defined($args[0]) && $args[0] ne "]") {
		print " Ar ", shift @args;
	}
	print "\n";
	shift @args;	# remove the "]"
}

while (<>) {
	chomp;
	next if length == 0;
	if (/^<H4>/i) {
		chew("</H4>");
		detag;
		tr/a-z/A-Z/;
		s/^\s+//;
		dehtmlchar;
		print ".Sh $_\n";
		next;
	}
	if (/<ADDRESS>/i) {
		chew("</ADDRESS>");
		detag;
		s/^\s+//;
		dehtmlchar;
		print ".Sh AUTHOR\n$_\n";
		next;
	}
	if (/<DL>/i) {
		print ".Bl -tag -width indent\n";
		next;
	}
	if (m#</DL>#i) {
		print ".El\n";
		next;
	}
	if (/<DT>/i) {
		chew("</DT>");
		detag;
		s/^\s*-//;
		s/ / Ar /;
		dehtmlchar;
		print ".It Fl $_\n";
		next;
	}
	if (/<TT>-/) {
		# command line option
		chew("</TT>");
		s#<TT>-([^<]*)#\n.Fl $1\n#ig;
		s#</TT>##ig;
		s#<I>([^<]+)</I>#\n.Ar $1\n#ig;
	}
	if (/<A HREF/) {
		# html reference to another ntp page
		chew("</A>");
		chomp($_ .= " " . <>) if (/<A$/);	# another reference on the next line
		s#<TT><A HREF="(.*).htm">\1</A></TT>#\n.Xr \1 $sec\n#ig;
		s#<A HREF="([^"]*)">(.*?) +</A> *page#\n.%T "$2"\npage in\n.Pa /usr/share/doc/html/ntp/$1\n#ig;
		s#^<BR>##g;
		s#<A HREF="([^"]*)">(.*?)</A>#For\n.%T "$2"\n, refer to\n.Pa /usr/share/doc/html/ntp/$1 .\n.Pp\n#ig;
		detag;
	}
	s#<TT>$name</TT>#\n.Nm\n#ig;
	s#<TT>([^<]*)</TT>#\n.Pa $1\n#ig;
	s#<TT><A HREF=[^>]*>([^<]*)</A></TT>#\n.Pa $1\n#ig;
	s#<PRE>#.Pp\n.nf\n#ig;
	s#</PRE>#\n.fi\n.Pp\n#ig;
	s#<P>#\n.Pp\n#ig;
	s#<(HR|DD)>##ig;
	s#</(BODY|HTML|DD)>##ig;
	s#<I>([^<]+)</I>#\n.Ar $1\n#ig;
	dehtmlchar;
	s/^\s+//;
	s/\n+\s*/\n/g;
	s/\n.Nm\n([,\.:]) /\n.Nm "" $1\n/g;
	s/\n\.(Pa|%T) (.*)\n([,\.:]) /\n.$1 $2 $3\n/g;
	s/\n$//;
	print $_, "\n" if length > 0;
}
