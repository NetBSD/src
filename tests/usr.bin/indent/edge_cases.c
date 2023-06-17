/* $NetBSD: edge_cases.c,v 1.4 2023/06/17 22:09:24 rillig Exp $ */

/*
 * Tests for edge cases in the C programming language that indent does not
 * support or in which cases indent behaves strangely.
 */

/*
 * Digraphs are replacements for the characters '[', '{' and '#', which are
 * missing in some exotic restricted source character sets.  They are not used
 * in practice, therefore indent doesn't need to support them.
 *
 * See C99 6.4.6
 */
//indent input
void
digraphs(void)
{
	/* same as 'array[subscript]' */
	number = array<:subscript:>;

	/* same as '(int){ initializer }' */
	number = (int)<% initializer %>;
}
//indent end

//indent run
void
digraphs(void)
{
	/* same as 'array[subscript]' */
// $ Indent interprets everything before the second ':' as a label name,
// $ indenting the "label" 2 levels to the left.
// $
// $ The space between 'array' and '<' comes from the binary operator '<'.
number = array <:subscript: >;

	/* same as '(int){ initializer }' */
// $ The opening '<' and '%' are interpreted as unary operators.
// $ The closing '%' and '>' are interpreted as a binary and unary operator.
	number = (int)<%initializer % >;
}
//indent end

/* TODO: test trigraphs, which are as unusual as digraphs */
/* TODO: test digraphs and trigraphs in string literals, just for fun */


/*
 * The keywords 'break', 'continue', 'goto' and 'restrict' are ordinary words,
 * they do not force a line break before.
 */
//indent input
{
	Whether to break or not to break, that is the question;

	The people goto the shopping mall;

	Begin at the beginning, then continue until you come to the end;
	then stop;

	Try to restrict yourself;
}
//indent end

//indent run-equals-input -di0


/*
 * Try a bit of Perl code, just for fun, taken from pkgsrc/pkgtools/pkglint4.
 *
 * It works surprisingly well.
 */
//indent input
package PkgLint::Line;

use strict;
use warnings;

BEGIN {
	import PkgLint::Util qw(
		false true
		assert
	);
}

use enum qw(FNAME LINES TEXT PHYSLINES CHANGED BEFORE AFTER EXTRA);

sub new($$$$) {
	my ($class, $fname, $lines, $text, $physlines) = @_;
	my ($self) = ([$fname, $lines, $text, $physlines, false, [], [], {}]);
	bless($self, $class);
	return $self;
}

sub fname($)		{ return shift()->[FNAME]; }

# querying, getting and setting the extra values.
sub has($$) {
	my ($self, $name) = @_;
	return exists($self->[EXTRA]->{$name});
}
//indent end

//indent run -di0 -nfbs -npsl
// $ Space after '::'.
package PkgLint:: Line;

use strict;
use warnings;

BEGIN {
// $ Space after '::'.
	import PkgLint:: Util qw(
				 false true
				 assert
	);
}

// $ Space between 'qw' and '('.
use enum qw (FNAME LINES TEXT PHYSLINES CHANGED BEFORE AFTER EXTRA);

sub new($$$$) {
// $ No space between 'my' and '('.
	my($class, $fname, $lines, $text, $physlines) = @_;
	my($self) = ([$fname, $lines, $text, $physlines, false, [], [], {
// $ Line break between '{' and '}'.
	}
// $ Line break between '}' and ']'.
	]);
	bless($self, $class);
	return $self;
}

sub fname($) {
	return shift()->[FNAME];
}

// $ Preprocessing lines are mostly preserved.
# querying, getting and setting the extra values.
sub has($$) {
	my($self, $name) = @_;
	return exists($self->[EXTRA]->{
// $ Line breaks between '{', '$name', '}' and ');'.
		$name
	}
	);
}
// exit 1
// error: Standard Input:17: Unbalanced parentheses
// warning: Standard Input:17: Extra ']'
// warning: Standard Input:17: Extra ')'
// error: Standard Input:27: Unbalanced parentheses
// warning: Standard Input:27: Extra ')'
//indent end


/*
 * Try a piece of old-style JavaScript, just for fun, using '==' instead of the
 * now recommended '==='.
 */
//indent input
function join(delim, values)
{
	if (values.length == 0)
		return '';
	if (values.length == 1)
		return values[0];
	var result = '';
	for (var i in values) {
		result += delim;
		result += values[i];
	}
	return result.substr(delim.length);
}
//indent end

//indent run-equals-input -di0 -npsl
