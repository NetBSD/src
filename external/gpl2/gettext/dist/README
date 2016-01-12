This is the GNU gettext package.  It is interesting for authors or
maintainers of other packages or programs which they want to see
internationalized.  As one step the handling of messages in different
languages should be implemented.  For this task GNU gettext provides
the needed tools and library functions.

Users of GNU packages should also install GNU gettext because some
other GNU packages will use the gettext program included in this
package to internationalize the messages given by shell scripts.

Another good reason to install GNU gettext is to make sure the
here included functions compile ok.  This helps to prevent errors
when installing other packages which use this library.  The message
handling functions are not yet part of POSIX and ISO/IEC standards
and therefore it is not possible to rely on facts about their
implementation in the local C library.  For this reason, GNU gettext
tries using the system's functionality only if it is a GNU gettext
implementation (possibly a different version); otherwise, compatibility
problems would occur.

We felt that the Uniforum proposals has the much more flexible interface
and, what is more important, does not burden the programmers as much as
the other possibility does.


Please share your results with us.  If this package compiles ok for
you future GNU release will likely also not fail, at least for reasons
found in message handling.  Send comments and bug reports to
		bug-gnu-gettext@gnu.org


The goal of this library was to give a unique interface to message
handling functions.  At least the same level of importance was to give
the programmer/maintainer the needed tools to maintain the message
catalogs.  The interface is designed after the proposals of the
Uniforum group.


The homepage of this package is at

           http://www.gnu.org/software/gettext/

The primary FTP site for its distribution is

           ftp://ftp.gnu.org/pub/gnu/gettext/


The configure script provides two non-standard options.  These will
also be available in other packages if they use the functionality of
GNU gettext.  Use

	--disable-nls

if you absolutely don't want to have messages handling code.  You will
always get the original messages (mostly English).  You could consider
using NLS support even when you do not need other tongues.  If you do
not install any messages catalogs or do not specify to use another but
the C locale you will not get translations.

The set of languages for which catalogs should be installed can also be
specified while configuring.  Of course they must be available but the
intersection of these two sets are computed automatically.  You could
once and for all define in your profile/cshrc the variable LINGUAS:

(Bourne Shell)		LINGUAS="de fr nl"; export LINGUAS

(C Shell)		setenv LINGUAS "de fr nl"

or specify it directly while configuring

	env LINGUAS="de fr nl" ./configure

Consult the manual for more information on language names.

The second configure option is

	--with-included-gettext

This forces to use the GNU implementation of the message handling library
regardless what the local C library provides.  This possibility is
useful if the local C library is a glibc 2.1.x or older, which didn't
have all the features the included libintl has.


Other files you might look into:

`ABOUT-NLS' -	current state of the GNU internationalization effort
`COPYING' -	copying conditions
`INSTALL' -	general compilation and installation rules
`NEWS' -	major changes in the current version
`THANKS' -	list of contributors


Some points you might be interested in before installing the package:

1.  If your system's C library already provides the gettext interface
    and its associated tools don't come from this package, it might be
    a good idea to configure the package with
    --program-transform-name='s/^gettext$/g&/;s/^msgfmt$/g&/;s/^xgettext$/g&/'

    Systems affected by this are:
        Solaris 2.x

2.  Some system have a very dumb^H^H^H^Hstrange version of msgfmt, the
    one which comes with xview.  This one is *not* usable.  It's best
    you delete^H^H^H^H^H^Hrename it or install this package as in the
    point above with
    --program-transform-name='s/^gettext$/g&/;s/^msgfmt$/g&/;s/^xgettext$/g&/'

3.  The locale name alias scheme implemented here is in a similar form
    implemented in the X Window System.  Especially the alias data base
    file can be shared.  Normally this file is found at something like

	/usr/lib/X11/locale/locale.alias

    If you have the X Window System installed try to find this file and
    specify the path at the make run:

    make aliaspath='/usr/lib/X11/locale:/usr/local/lib/locale'

    (or whatever is appropriate for you).  The file name is always
    locale.alias.
    In the misc/ subdirectory you find an example for an alias database file.

4.  The msgmerge program performs fuzzy search in the message sets.  It
    might run a long time on slow systems.  I saw this problem when running
    it on my old i386DX25.  The time can really be several minutes,
    especially if you have long messages and/or a great number of
    them.
       If you have a faster implementation of the fstrcmp() function and
    want to share it with the rest of us, please contact me.
