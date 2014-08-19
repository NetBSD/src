sinclude(libtool.m4/libtool.m4)dnl
sinclude(libtool.m4/ltoptions.m4)dnl
sinclude(libtool.m4/ltsugar.m4)dnl
sinclude(libtool.m4/ltversion.m4)dnl
sinclude(libtool.m4/lt~obsolete.m4)dnl

m4_divert_text(HELP_CANON, [[
  NOTE: If PREFIX is not set, then the default values for --sysconfdir
  and --localstatedir are /etc and /var, respectively.]])
m4_divert_text(HELP_END, [[
Professional support for BIND is provided by Internet Systems Consortium,
Inc., doing business as DNSco.  Information about paid support options is
available at http://www.dns-co.com/solutions/.  Free support is provided by
our user community via a mailing list.  Information on public email lists
is available at https://www.isc.org/community/mailing-list/.]])
