# Toying at an interface between Perl and GNU gettext .mo format.
# Copyright (C) 1995 Free Software Foundation, Inc.
# François Pinard <pinard@iro.umontreal.ca>, 1995.

textdomain ("tar");
print &_("Try \`%s --help\' for more information.\n");
exit 0;

## --------------------------------------------------------------- ##
## The `&textdomain (DOMAIN_NAME)' routine reads the given domain  ##
## into an associative array %_, able to later translate strings.  ##
## --------------------------------------------------------------- ##

sub textdomain
{
    local ($language, $catalog, $domain, $buffer);
    local ($reverse);
    local ($magic, $revision, $nstrings, $orig_tab_offset, $trans_tab_offset);
    local ($orig_length, $orig_pointer, $trans_length, $trans_pointer);

    %_ = ();

    $language = $ENV{"LANG"};
    return if ! $language;
    $domain = $_[0];
    $catalog = "/usr/local/share/locale/$language/LC_MESSAGES/$domain.mo";

    open (CATALOG, $catalog) || return;
    sysread (CATALOG, $buffer, (stat CATALOG)[7]);
    close CATALOG;

    $magic = unpack ("I", $buffer);
    if (sprintf ("%x", $magic) eq "de120495")
    {
	$reverse = 1;
    }
    elsif (sprintf ("%x", $magic) ne "950412de")
    {
	die "Not a catalog file\n";
    }

    $revision = &mo_format_value (4);
    $nstrings = &mo_format_value (8);
    $orig_tab_offset = &mo_format_value (12);
    $trans_tab_offset = &mo_format_value (16);

    while ($nstrings-- > 0)
    {
	$orig_length = &mo_format_value ($orig_tab_offset);
	$orig_pointer = &mo_format_value ($orig_tab_offset + 4);
	$orig_tab_offset += 8;

	$trans_length = &mo_format_value ($trans_tab_offset);
	$trans_pointer = &mo_format_value ($trans_tab_offset + 4);
	$trans_tab_offset += 8;

	$_{substr ($buffer, $orig_pointer, $orig_length)}
	    = substr ($buffer, $trans_pointer, $trans_length);
    }
}

## ----------------------------------------------------------------- ##
## The `&mo_format_value (ADDRESS)' routine returns the value at a   ##
## given address in the .mo format catalog, once read into $buffer   ##
## by `&textdomain'.  This is a service routine of `&textdomain',    ##
## which uses $buffer and $reverse variables local in that routine.  ##
## ----------------------------------------------------------------- ##

sub mo_format_value
{
    unpack ("i",
	    $reverse
	    ? pack ("c4", reverse unpack ("c4", substr ($buffer, $_[0], 4)))
	    : substr ($buffer, $_[0], 4));
}

## ------------------------------------------------------------ ##
## The `&_(STRING)' routine translates STRING if there is some  ##
## translation offered for it in the `%_' associative array, or ##
## return STRING itself, otherwize.			        ##
## ------------------------------------------------------------ ##

sub _
{
    defined $_{$_[0]} ? $_{$_[0]} : $_[0];
}
