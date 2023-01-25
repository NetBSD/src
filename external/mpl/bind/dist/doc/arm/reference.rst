.. Copyright (C) Internet Systems Consortium, Inc. ("ISC")
..
.. SPDX-License-Identifier: MPL-2.0
..
.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0.  If a copy of the MPL was not distributed with this
.. file, you can obtain one at https://mozilla.org/MPL/2.0/.
..
.. See the COPYRIGHT file distributed with this work for additional
.. information regarding copyright ownership.

.. Reference:

BIND 9 Configuration Reference
==============================

.. _configuration_file_elements:

Configuration File Elements
---------------------------

Following is a list of elements used throughout the BIND configuration
file documentation:

.. glossary::

    ``acl_name``
        The name of an ``address_match_list`` as defined by the ``acl`` statement.

    ``address_match_list``
        A list of one or more ``ip_addr``, ``ip_prefix``, ``key_id``, or ``acl_name`` elements; see :ref:`address_match_lists`.

    ``remoteserver_list``
        A named list of one or more ``ip_addr`` with optional ``key_id`` and/or ``ip_port``. A ``remoteserver_list`` may include other ``remoteserver_list``.

    ``domain_name``
        A quoted string which is used as a DNS name; for example. ``my.test.domain``.

    ``namelist``
        A list of one or more ``domain_name`` elements.

    ``dotted_decimal``
        One to four integers valued 0 through 255 separated by dots (``.``), such as ``123.45.67`` or ``89.123.45.67``.

    ``ip4_addr``
        An IPv4 address with exactly four elements in ``dotted_decimal`` notation.

    ``ip6_addr``
        An IPv6 address, such as ``2001:db8::1234``. IPv6-scoped addresses that have ambiguity on their scope zones must be disambiguated by an appropriate zone ID with the percent character (``%``) as a delimiter. It is strongly recommended to use string zone names rather than numeric identifiers, to be robust against system configuration changes. However, since there is no standard mapping for such names and identifier values, only interface names as link identifiers are supported, assuming one-to-one mapping between interfaces and links. For example, a link-local address ``fe80::1`` on the link attached to the interface ``ne0`` can be specified as ``fe80::1%ne0``. Note that on most systems link-local addresses always have ambiguity and need to be disambiguated.

    ``ip_addr``
        An ``ip4_addr`` or ``ip6_addr``.

    ``ip_dscp``
        A ``number`` between 0 and 63, used to select a differentiated services code point (DSCP) value for use with outgoing traffic on operating systems that support DSCP.

    ``ip_port``
        An IP port ``number``. The ``number`` is limited to 0 through 65535, with values below 1024 typically restricted to use by processes running as root. In some cases, an asterisk (``*``) character can be used as a placeholder to select a random high-numbered port.

    ``ip_prefix``
        An IP network specified as an ``ip_addr``, followed by a slash (``/``) and then the number of bits in the netmask. Trailing zeros in an``ip_addr`` may be omitted. For example, ``127/8`` is the network ``127.0.0.0``with netmask ``255.0.0.0`` and ``1.2.3.0/28`` is network ``1.2.3.0`` with netmask ``255.255.255.240``.
        When specifying a prefix involving a IPv6-scoped address, the scope may be omitted. In that case, the prefix matches packets from any scope.

    ``key_id``
        A ``domain_name`` representing the name of a shared key, to be used for transaction security.

    ``key_list``
        A list of one or more ``key_id``, separated by semicolons and ending with a semicolon.

    ``number``
        A non-negative 32-bit integer (i.e., a number between 0 and 4294967295, inclusive). Its acceptable value might be further limited by the context in which it is used.

    ``fixedpoint``
        A non-negative real number that can be specified to the nearest one-hundredth. Up to five digits can be specified before a decimal point, and up to two digits after, so the maximum value is 99999.99. Acceptable values might be further limited by the contexts in which they are used.

    ``path_name``
        A quoted string which is used as a pathname, such as ``zones/master/my.test.domain``.

    ``port_list``
        A list of an ``ip_port`` or a port range. A port range is specified in the form of ``range`` followed by two ``ip_port``s, ``port_low`` and ``port_high``, which represents port numbers from ``port_low`` through ``port_high``, inclusive. ``port_low`` must not be larger than ``port_high``. For example, ``range 1024 65535`` represents ports from 1024 through 65535. In either case an asterisk (``*``) character is not allowed as a valid ``ip_port``.

    ``size_spec``
        A 64-bit unsigned integer, or the keywords ``unlimited`` or ``default``. Integers may take values 0 <= value <= 18446744073709551615, though certain parameters (such as ``max-journal-size``) may use a more limited range within these extremes. In most cases, setting a value to 0 does not literally mean zero; it means "undefined" or "as big as possible," depending on the context. See the explanations of particular parameters that use ``size_spec`` for details on how they interpret its use. Numeric values can optionally be followed by a scaling factor: ``K`` or ``k`` for kilobytes, ``M`` or ``m`` for megabytes, and ``G`` or ``g`` for gigabytes, which scale by 1024, 1024*1024, and 1024*1024*1024 respectively.
        ``unlimited`` generally means "as big as possible," and is usually the best way to safely set a very large number.
        ``default`` uses the limit that was in force when the server was started.

    ``size_or_percent``
         A ``size_spec`` or integer value followed by ``%`` to represent percent. The behavior is exactly the same as ``size_spec``, but ``size_or_percent`` also allows specifying a positive integer value followed by the ``%`` sign to represent percent.

    ``yes_or_no``
        Either ``yes`` or ``no``. The words ``true`` and ``false`` are also accepted, as are the numbers ``1`` and ``0``.

    ``dialup_option``
        One of ``yes``, ``no``, ``notify``, ``notify-passive``, ``refresh``, or  ``passive``. When used in a zone, ``notify-passive``, ``refresh``, and ``passive`` are restricted to secondary and stub zones.

.. _address_match_lists:

Address Match Lists
~~~~~~~~~~~~~~~~~~~

Syntax
^^^^^^

::

   address_match_list = address_match_list_element ; ...

   address_match_list_element = [ ! ] ( ip_address | ip_prefix |
        key key_id | acl_name | { address_match_list } )

Definition and Usage
^^^^^^^^^^^^^^^^^^^^

Address match lists are primarily used to determine access control for
various server operations. They are also used in the ``listen-on`` and
``sortlist`` statements. The elements which constitute an address match
list can be any of the following:

-  an IP address (IPv4 or IPv6)

-  an IP prefix (in ``/`` notation)

-  a key ID, as defined by the ``key`` statement

-  the name of an address match list defined with the ``acl`` statement

-  a nested address match list enclosed in braces

Elements can be negated with a leading exclamation mark (``!``), and the
match list names "any", "none", "localhost", and "localnets" are
predefined. More information on those names can be found in the
description of the ``acl`` statement.

The addition of the key clause made the name of this syntactic element
something of a misnomer, since security keys can be used to validate
access without regard to a host or network address. Nonetheless, the
term "address match list" is still used throughout the documentation.

When a given IP address or prefix is compared to an address match list,
the comparison takes place in approximately O(1) time. However, key
comparisons require that the list of keys be traversed until a matching
key is found, and therefore may be somewhat slower.

The interpretation of a match depends on whether the list is being used
for access control, defining ``listen-on`` ports, or in a ``sortlist``,
and whether the element was negated.

When used as an access control list, a non-negated match allows access
and a negated match denies access. If there is no match, access is
denied. The clauses ``allow-notify``, ``allow-recursion``,
``allow-recursion-on``, ``allow-query``, ``allow-query-on``,
``allow-query-cache``, ``allow-query-cache-on``, ``allow-transfer``,
``allow-update``, ``allow-update-forwarding``, ``blackhole``, and
``keep-response-order`` all use address match lists. Similarly, the
``listen-on`` option causes the server to refuse queries on any of
the machine's addresses which do not match the list.

Order of insertion is significant. If more than one element in an ACL is
found to match a given IP address or prefix, preference is given to
the one that came *first* in the ACL definition. Because of this
first-match behavior, an element that defines a subset of another
element in the list should come before the broader element, regardless
of whether either is negated. For example, in ``1.2.3/24; ! 1.2.3.13;``
the 1.2.3.13 element is completely useless because the algorithm
matches any lookup for 1.2.3.13 to the 1.2.3/24 element. Using
``! 1.2.3.13; 1.2.3/24`` fixes that problem by blocking 1.2.3.13
via the negation, but all other 1.2.3.\* hosts pass through.

.. _comment_syntax:

Comment Syntax
~~~~~~~~~~~~~~

The BIND 9 comment syntax allows comments to appear anywhere that
whitespace may appear in a BIND configuration file. To appeal to
programmers of all kinds, they can be written in the C, C++, or
shell/perl style.

Syntax
^^^^^^

::

   /* This is a BIND comment as in C */

::

   // This is a BIND comment as in C++

::

   # This is a BIND comment as in common Unix shells
   # and perl

Definition and Usage
^^^^^^^^^^^^^^^^^^^^

Comments may appear anywhere that whitespace may appear in a BIND
configuration file.

C-style comments start with the two characters /\* (slash, star) and end
with \*/ (star, slash). Because they are completely delimited with these
characters, they can be used to comment only a portion of a line or to
span multiple lines.

C-style comments cannot be nested. For example, the following is not
valid because the entire comment ends with the first \*/:

::

   /* This is the start of a comment.
      This is still part of the comment.
   /* This is an incorrect attempt at nesting a comment. */
      This is no longer in any comment. */

C++-style comments start with the two characters // (slash, slash) and
continue to the end of the physical line. They cannot be continued
across multiple physical lines; to have one logical comment span
multiple lines, each line must use the // pair. For example:

::

   // This is the start of a comment.  The next line
   // is a new comment, even though it is logically
   // part of the previous comment.

Shell-style (or perl-style) comments start with the
character ``#`` (number sign) and continue to the end of the physical
line, as in C++ comments. For example:

::

   # This is the start of a comment.  The next line
   # is a new comment, even though it is logically
   # part of the previous comment.

..

.. warning::

   The semicolon (``;``) character cannot start a comment, unlike
   in a zone file. The semicolon indicates the end of a
   configuration statement.

.. _Configuration_File_Grammar:

Configuration File Grammar
--------------------------

A BIND 9 configuration consists of statements and comments. Statements
end with a semicolon; statements and comments are the only elements that
can appear without enclosing braces. Many statements contain a block of
sub-statements, which are also terminated with a semicolon.

The following statements are supported:

    ``acl``
        Defines a named IP address matching list, for access control and other uses.

    ``controls``
        Declares control channels to be used by the ``rndc`` utility.

    ``dnssec-policy``
        Describes a DNSSEC key and signing policy for zones. See :ref:`dnssec-policy Grammar <dnssec_policy_grammar>` for details.

    ``include``
        Includes a file.

    ``key``
        Specifies key information for use in authentication and authorization using TSIG.

    ``logging``
        Specifies what information the server logs and where the log messages are sent.

    ``masters``
        Synonym for ``primaries``.

    ``options``
        Controls global server configuration options and sets defaults for other statements.

    ``parental-agents``
        Defines a named list of servers for inclusion in primary and secondary zones' ``parental-agents`` lists.

    ``primaries``
        Defines a named list of servers for inclusion in stub and secondary zones' ``primaries`` or ``also-notify`` lists. (Note: this is a synonym for the original keyword ``masters``, which can still be used, but is no longer the preferred terminology.)

    ``server``
        Sets certain configuration options on a per-server basis.

    ``statistics-channels``
        Declares communication channels to get access to ``named`` statistics.

    ``trust-anchors``
        Defines DNSSEC trust anchors: if used with the ``initial-key`` or ``initial-ds`` keyword, trust anchors are kept up-to-date using :rfc:`5011` trust anchor maintenance; if used with ``static-key`` or ``static-ds``, keys are permanent.

    ``managed-keys``
        Is identical to ``trust-anchors``; this option is deprecated in favor of ``trust-anchors`` with the ``initial-key`` keyword, and may be removed in a future release.

    ``trusted-keys``
        Defines permanent trusted DNSSEC keys; this option is deprecated in favor of ``trust-anchors`` with the ``static-key`` keyword, and may be removed in a future release.

    ``view``
        Defines a view.

    ``zone``
        Defines a zone.

The ``logging`` and ``options`` statements may only occur once per
configuration.

.. _acl_grammar:

``acl`` Statement Grammar
~~~~~~~~~~~~~~~~~~~~~~~~~

.. include:: ../misc/acl.grammar.rst

.. _acl:

``acl`` Statement Definition and Usage
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ``acl`` statement assigns a symbolic name to an address match list.
It gets its name from one of the primary uses of address match lists: Access
Control Lists (ACLs).

The following ACLs are built-in:

    ``any``
        Matches all hosts.

    ``none``
        Matches no hosts.

    ``localhost``
        Matches the IPv4 and IPv6 addresses of all network interfaces on the system. When addresses are added or removed, the ``localhost`` ACL element is updated to reflect the changes.

    ``localnets``
        Matches any host on an IPv4 or IPv6 network for which the system has an interface. When addresses are added or removed, the ``localnets`` ACL element is updated to reflect the changes. Some systems do not provide a way to determine the prefix lengths of local IPv6 addresses; in such cases, ``localnets`` only matches the local IPv6 addresses, just like ``localhost``.

.. _controls_grammar:

``controls`` Statement Grammar
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. include:: ../misc/controls.grammar.rst

.. _controls_statement_definition_and_usage:

``controls`` Statement Definition and Usage
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ``controls`` statement declares control channels to be used by
system administrators to manage the operation of the name server. These
control channels are used by the ``rndc`` utility to send commands to
and retrieve non-DNS results from a name server.

An ``inet`` control channel is a TCP socket listening at the specified
``ip_port`` on the specified ``ip_addr``, which can be an IPv4 or IPv6
address. An ``ip_addr`` of ``*`` (asterisk) is interpreted as the IPv4
wildcard address; connections are accepted on any of the system's
IPv4 addresses. To listen on the IPv6 wildcard address, use an
``ip_addr`` of ``::``. If ``rndc`` is only used on the local host,
using the loopback address (``127.0.0.1`` or ``::1``) is recommended for
maximum security.

If no port is specified, port 953 is used. The asterisk ``*`` cannot
be used for ``ip_port``.

The ability to issue commands over the control channel is restricted by
the ``allow`` and ``keys`` clauses. Connections to the control channel
are permitted based on the ``address_match_list``. This is for simple IP
address-based filtering only; any ``key_id`` elements of the
``address_match_list`` are ignored.

A ``unix`` control channel is a Unix domain socket listening at the
specified path in the file system. Access to the socket is specified by
the ``perm``, ``owner``, and ``group`` clauses. Note that on some platforms
(SunOS and Solaris), the permissions (``perm``) are applied to the parent
directory as the permissions on the socket itself are ignored.

The primary authorization mechanism of the command channel is the
``key_list``, which contains a list of ``key_id``s. Each ``key_id`` in
the ``key_list`` is authorized to execute commands over the control
channel. See :ref:`admin_tools` for information about
configuring keys in ``rndc``.

If the ``read-only`` clause is enabled, the control channel is limited
to the following set of read-only commands: ``nta -dump``, ``null``,
``status``, ``showzone``, ``testgen``, and ``zonestatus``. By default,
``read-only`` is not enabled and the control channel allows read-write
access.

If no ``controls`` statement is present, ``named`` sets up a default
control channel listening on the loopback address 127.0.0.1 and its IPv6
counterpart, ::1. In this case, and also when the ``controls`` statement
is present but does not have a ``keys`` clause, ``named`` attempts
to load the command channel key from the file ``rndc.key`` in ``/etc``
(or whatever ``sysconfdir`` was specified when BIND was built). To
create an ``rndc.key`` file, run ``rndc-confgen -a``.

To disable the command channel, use an empty ``controls`` statement:
``controls { };``.

.. _include_grammar:

``include`` Statement Grammar
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

::

   include filename;

.. _include_statement:

``include`` Statement Definition and Usage
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ``include`` statement inserts the specified file (or files if a valid glob
expression is detected) at the point where the ``include`` statement is
encountered. The ``include`` statement facilitates the administration of
configuration files by permitting the reading or writing of some things but not
others. For example, the statement could include private keys that are readable
only by the name server.

.. _key_grammar:

``key`` Statement Grammar
~~~~~~~~~~~~~~~~~~~~~~~~~

.. include:: ../misc/key.grammar.rst

.. _key_statement:

``key`` Statement Definition and Usage
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ``key`` statement defines a shared secret key for use with TSIG (see
:ref:`tsig`) or the command channel (see :ref:`controls_statement_definition_and_usage`).

The ``key`` statement can occur at the top level of the configuration
file or inside a ``view`` statement. Keys defined in top-level ``key``
statements can be used in all views. Keys intended for use in a
``controls`` statement (see :ref:`controls_statement_definition_and_usage`)
must be defined at the top level.

The ``key_id``, also known as the key name, is a domain name that uniquely
identifies the key. It can be used in a ``server`` statement to cause
requests sent to that server to be signed with this key, or in address
match lists to verify that incoming requests have been signed with a key
matching this name, algorithm, and secret.

The ``algorithm_id`` is a string that specifies a security/authentication
algorithm. The ``named`` server supports ``hmac-md5``, ``hmac-sha1``,
``hmac-sha224``, ``hmac-sha256``, ``hmac-sha384``, and ``hmac-sha512``
TSIG authentication. Truncated hashes are supported by appending the
minimum number of required bits preceded by a dash, e.g.,
``hmac-sha1-80``. The ``secret_string`` is the secret to be used by the
algorithm, and is treated as a Base64-encoded string.

.. _logging_grammar:

``logging`` Statement Grammar
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. include:: ../misc/logging.grammar.rst

.. _logging_statement:

``logging`` Statement Definition and Usage
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ``logging`` statement configures a wide variety of logging options
for the name server. Its ``channel`` phrase associates output methods,
format options, and severity levels with a name that can then be used
with the ``category`` phrase to select how various classes of messages
are logged.

Only one ``logging`` statement is used to define as many channels and
categories as desired. If there is no ``logging`` statement, the
logging configuration is:

::

   logging {
        category default { default_syslog; default_debug; };
        category unmatched { null; };
   };

If ``named`` is started with the ``-L`` option, it logs to the specified
file at startup, instead of using syslog. In this case the logging
configuration is:

::

   logging {
        category default { default_logfile; default_debug; };
        category unmatched { null; };
   };

The logging configuration is only established when the entire
configuration file has been parsed. When the server starts up, all
logging messages regarding syntax errors in the configuration file go to
the default channels, or to standard error if the ``-g`` option was
specified.

.. _channel:

The ``channel`` Phrase
^^^^^^^^^^^^^^^^^^^^^^

All log output goes to one or more ``channels``; there is no limit to
the number of channels that can be created.

Every channel definition must include a destination clause that says
whether messages selected for the channel go to a file, go to a particular
syslog facility, go to the standard error stream, or are discarded. The definition can
optionally also limit the message severity level that is accepted
by the channel (the default is ``info``), and whether to include a
``named``-generated time stamp, the category name, and/or the severity level
(the default is not to include any).

The ``null`` destination clause causes all messages sent to the channel
to be discarded; in that case, other options for the channel are
meaningless.

The ``file`` destination clause directs the channel to a disk file. It
can include additional arguments to specify how large the file is
allowed to become before it is rolled to a backup file (``size``), how
many backup versions of the file are saved each time this happens
(``versions``), and the format to use for naming backup versions
(``suffix``).

The ``size`` option is used to limit log file growth. If the file ever
exceeds the specified size, then ``named`` stops writing to the file
unless it has a ``versions`` option associated with it. If backup
versions are kept, the files are rolled as described below. If there is
no ``versions`` option, no more data is written to the log until
some out-of-band mechanism removes or truncates the log to less than the
maximum size. The default behavior is not to limit the size of the file.

File rolling only occurs when the file exceeds the size specified with
the ``size`` option. No backup versions are kept by default; any
existing log file is simply appended. The ``versions`` option specifies
how many backup versions of the file should be kept. If set to
``unlimited``, there is no limit.

The ``suffix`` option can be set to either ``increment`` or
``timestamp``. If set to ``timestamp``, then when a log file is rolled,
it is saved with the current timestamp as a file suffix. If set to
``increment``, then backup files are saved with incrementing numbers as
suffixes; older files are renamed when rolling. For example, if
``versions`` is set to 3 and ``suffix`` to ``increment``, then when
``filename.log`` reaches the size specified by ``size``,
``filename.log.1`` is renamed to ``filename.log.2``, ``filename.log.0``
is renamed to ``filename.log.1``, and ``filename.log`` is renamed to
``filename.log.0``, whereupon a new ``filename.log`` is opened.

Here is an example using the ``size``, ``versions``, and ``suffix`` options:

::

   channel an_example_channel {
       file "example.log" versions 3 size 20m suffix increment;
       print-time yes;
       print-category yes;
   };

The ``syslog`` destination clause directs the channel to the system log.
Its argument is a syslog facility as described in the ``syslog`` man
page. Known facilities are ``kern``, ``user``, ``mail``, ``daemon``,
``auth``, ``syslog``, ``lpr``, ``news``, ``uucp``, ``cron``,
``authpriv``, ``ftp``, ``local0``, ``local1``, ``local2``, ``local3``,
``local4``, ``local5``, ``local6``, and ``local7``; however, not all
facilities are supported on all operating systems. How ``syslog`` 
handles messages sent to this facility is described in the
``syslog.conf`` man page. On a system which uses a very old
version of ``syslog``, which only uses two arguments to the ``openlog()``
function, this clause is silently ignored.

On Windows machines, syslog messages are directed to the EventViewer.

The ``severity`` clause works like ``syslog``'s "priorities," except
that they can also be used when writing straight to a file rather
than using ``syslog``. Messages which are not at least of the severity
level given are not selected for the channel; messages of higher
severity levels are accepted.

When using ``syslog``, the ``syslog.conf`` priorities
also determine what eventually passes through. For example, defining a
channel facility and severity as ``daemon`` and ``debug``, but only
logging ``daemon.warning`` via ``syslog.conf``, causes messages of
severity ``info`` and ``notice`` to be dropped. If the situation were
reversed, with ``named`` writing messages of only ``warning`` or higher,
then ``syslogd`` would print all messages it received from the channel.

The ``stderr`` destination clause directs the channel to the server's
standard error stream. This is intended for use when the server is
running as a foreground process, as when debugging a
configuration, for example.

The server can supply extensive debugging information when it is in
debugging mode. If the server's global debug level is greater than zero,
debugging mode is active. The global debug level is set either
by starting the ``named`` server with the ``-d`` flag followed by a
positive integer, or by running ``rndc trace``. The global debug level
can be set to zero, and debugging mode turned off, by running ``rndc
notrace``. All debugging messages in the server have a debug level;
higher debug levels give more detailed output. Channels that specify a
specific debug severity, for example:

::

   channel specific_debug_level {
       file "foo";
       severity debug 3;
   };

get debugging output of level 3 or less any time the server is in
debugging mode, regardless of the global debugging level. Channels with
``dynamic`` severity use the server's global debug level to determine
what messages to print.

``print-time`` can be set to ``yes``, ``no``, or a time format
specifier, which may be one of ``local``, ``iso8601``, or
``iso8601-utc``. If set to ``no``, the date and time are not
logged. If set to ``yes`` or ``local``, the date and time are logged in
a human-readable format, using the local time zone. If set to
``iso8601``, the local time is logged in ISO 8601 format. If set to
``iso8601-utc``, the date and time are logged in ISO 8601 format,
with time zone set to UTC. The default is ``no``.

``print-time`` may be specified for a ``syslog`` channel, but it is
usually pointless since ``syslog`` also logs the date and time.

If ``print-category`` is requested, then the category of the message
is logged as well. Finally, if ``print-severity`` is on, then the
severity level of the message is logged. The ``print-`` options may
be used in any combination, and are always printed in the following
order: time, category, severity. Here is an example where all three
``print-`` options are on:

``28-Feb-2000 15:05:32.863 general: notice: running``

If ``buffered`` has been turned on, the output to files is not
flushed after each log entry. By default all log messages are flushed.

There are four predefined channels that are used for ``named``'s default
logging, as follows. If ``named`` is started with the ``-L`` option, then a fifth
channel, ``default_logfile``, is added. How they are used is described in
:ref:`the_category_phrase`.

::

   channel default_syslog {
       // send to syslog's daemon facility
       syslog daemon;
       // only send priority info and higher
       severity info;
   };

   channel default_debug {
       // write to named.run in the working directory
       // Note: stderr is used instead of "named.run" if
       // the server is started with the '-g' option.
       file "named.run";
       // log at the server's current debug level
       severity dynamic;
   };

   channel default_stderr {
       // writes to stderr
       stderr;
       // only send priority info and higher
       severity info;
   };

   channel null {
      // toss anything sent to this channel
      null;
   };

   channel default_logfile {
       // this channel is only present if named is
       // started with the -L option, whose argument
       // provides the file name
       file "...";
       // log at the server's current debug level
       severity dynamic;
   };

The ``default_debug`` channel has the special property that it only
produces output when the server's debug level is non-zero. It normally
writes to a file called ``named.run`` in the server's working directory.

For security reasons, when the ``-u`` command-line option is used, the
``named.run`` file is created only after ``named`` has changed to the
new UID, and any debug output generated while ``named`` is starting -
and still running as root - is discarded. To capture this
output, run the server with the ``-L`` option to specify a
default logfile, or the ``-g`` option to log to standard error which can
be redirected to a file.

Once a channel is defined, it cannot be redefined. The
built-in channels cannot be altered directly, but the default logging
can be modified by pointing categories at defined channels.

.. _the_category_phrase:

The ``category`` Phrase
^^^^^^^^^^^^^^^^^^^^^^^

There are many categories, so desired logs can be sent anywhere
while unwanted logs are ignored. If
a list of channels is not specified for a category, log messages in that
category are sent to the ``default`` category instead. If no
default category is specified, the following "default default" is used:

::

   category default { default_syslog; default_debug; };

If ``named`` is started with the ``-L`` option, the default category
is:

::

   category default { default_logfile; default_debug; };

As an example, let's say a user wants to log security events to a file, but
also wants to keep the default logging behavior. They would specify the
following:

::

   channel my_security_channel {
       file "my_security_file";
       severity info;
   };
   category security {
       my_security_channel;
       default_syslog;
       default_debug;
   };

To discard all messages in a category, specify the ``null`` channel:

::

   category xfer-out { null; };
   category notify { null; };

The following are the available categories and brief descriptions of the
types of log information they contain. More categories may be added in
future BIND releases.

.. include:: logging-categories.rst

.. _query_errors:

The ``query-errors`` Category
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``query-errors`` category is used to indicate why and how specific queries
resulted in responses which indicate an error.  Normally, these messages are
logged at ``debug`` logging levels; note, however, that if query logging is
active, some are logged at ``info``. The logging levels are described below:

At ``debug`` level 1 or higher - or at ``info`` when query logging is
active - each response with the rcode of SERVFAIL is logged as follows:

``client 127.0.0.1#61502: query failed (SERVFAIL) for www.example.com/IN/AAAA at query.c:3880``

This means an error resulting in SERVFAIL was detected at line 3880 of source
file ``query.c``.  Log messages of this level are particularly helpful in identifying
the cause of SERVFAIL for an authoritative server.

At ``debug`` level 2 or higher, detailed context information about recursive
resolutions that resulted in SERVFAIL is logged.  The log message looks
like this:

::

   fetch completed at resolver.c:2970 for www.example.com/A
   in 10.000183: timed out/success [domain:example.com,
   referral:2,restart:7,qrysent:8,timeout:5,lame:0,quota:0,neterr:0,
   badresp:1,adberr:0,findfail:0,valfail:0]

The first part before the colon shows that a recursive resolution for
AAAA records of www.example.com completed in 10.000183 seconds, and the
final result that led to the SERVFAIL was determined at line 2970 of
source file ``resolver.c``.

The next part shows the detected final result and the latest result of
DNSSEC validation.  The latter is always "success" when no validation attempt
was made.  In this example, this query probably resulted in SERVFAIL because all
name servers are down or unreachable, leading to a timeout in 10 seconds.
DNSSEC validation was probably not attempted.

The last part, enclosed in square brackets, shows statistics collected for this
particular resolution attempt.  The ``domain`` field shows the deepest zone that
the resolver reached; it is the zone where the error was finally detected.  The
meaning of the other fields is summarized in the following list.

``referral``
    The number of referrals the resolver received throughout the resolution process. In the above ``example.com`` there are two.

``restart``
    The number of cycles that the resolver tried remote servers at the ``domain`` zone. In each cycle, the resolver sends one query (possibly resending it, depending on the response) to each known name server of the ``domain`` zone.

``qrysent``
    The number of queries the resolver sent at the ``domain`` zone.

``timeout``
    The number of timeouts the resolver received since the last response.

``lame``
    The number of lame servers the resolver detected at the ``domain`` zone. A server is detected to be lame either by an invalid response or as a result of lookup in BIND 9's address database (ADB), where lame servers are cached.

``quota``
    The number of times the resolver was unable to send a query because it had exceeded the permissible fetch quota for a server.

``neterr``
    The number of erroneous results that the resolver encountered in sending queries at the ``domain`` zone. One common case is when the remote server is unreachable and the resolver receives an "ICMP unreachable" error message.

``badresp``
    The number of unexpected responses (other than ``lame``) to queries sent by the resolver at the ``domain`` zone.

``adberr``
    Failures in finding remote server addresses of the``domain`` zone in the ADB. One common case of this is that the remote server's name does not have any address records.

``findfail``
    Failures to resolve remote server addresses. This is a total number of failures throughout the resolution process.

``valfail``
    Failures of DNSSEC validation. Validation failures are counted throughout the resolution process (not limited to the ``domain`` zone), but should only happen in ``domain``.

At ``debug`` level 3 or higher, the same messages as those at
``debug`` level 1 are logged for errors other than
SERVFAIL. Note that negative responses such as NXDOMAIN are not errors, and are
not logged at this debug level.

At ``debug`` level 4 or higher, the detailed context information logged at
``debug`` level 2 is logged for errors other than SERVFAIL and for negative
responses such as NXDOMAIN.

.. _parental_agents_grammar:

``parental-agents`` Statement Grammar
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. include:: ../misc/parental-agents.grammar.rst

.. _parental_agents_statement:

``parental-agents`` Statement Definition and Usage
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``parental-agents`` lists allow for a common set of parental agents to be easily
used by multiple primary and secondary zones in their ``parental-agents`` lists.
A parental agent is the entity that the zone has a relationship with to
change its delegation information (defined in :rfc:`7344`).

.. _primaries_grammar:

``primaries`` Statement Grammar
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. include:: ../misc/primaries.grammar.rst

.. _primaries_statement:

``primaries`` Statement Definition and Usage
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``primaries`` lists allow for a common set of primary servers  to be easily
used by multiple stub and secondary zones in their ``primaries`` or
``also-notify`` lists. (Note: ``primaries`` is a synonym for the original
keyword ``masters``, which can still be used, but is no longer the
preferred terminology.)

.. _options_grammar:

``options`` Statement Grammar
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This is the grammar of the ``options`` statement in the ``named.conf``
file:

.. include:: ../misc/options.grammar.rst

.. _options:

``options`` Statement Definition and Usage
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ``options`` statement sets up global options to be used by BIND.
This statement may appear only once in a configuration file. If there is
no ``options`` statement, an options block with each option set to its
default is used.

.. _attach-cache:

``attach-cache``
   This option allows multiple views to share a single cache database. Each view has
   its own cache database by default, but if multiple views have the
   same operational policy for name resolution and caching, those views
   can share a single cache to save memory, and possibly improve
   resolution efficiency, by using this option.

   The ``attach-cache`` option may also be specified in ``view``
   statements, in which case it overrides the global ``attach-cache``
   option.

   The ``cache_name`` specifies the cache to be shared. When the ``named``
   server configures views which are supposed to share a cache, it
   creates a cache with the specified name for the first view of these
   sharing views. The rest of the views simply refer to the
   already-created cache.

   One common configuration to share a cache is to allow all views
   to share a single cache. This can be done by specifying
   ``attach-cache`` as a global option with an arbitrary name.

   Another possible operation is to allow a subset of all views to share
   a cache while the others retain their own caches. For example, if
   there are three views A, B, and C, and only A and B should share a
   cache, specify the ``attach-cache`` option as a view of A (or B)'s
   option, referring to the other view name:

   ::

        view "A" {
          // this view has its own cache
          ...
        };
        view "B" {
          // this view refers to A's cache
          attach-cache "A";
        };
        view "C" {
          // this view has its own cache
          ...
        };

   Views that share a cache must have the same policy on configurable
   parameters that may affect caching. The current implementation
   requires the following configurable options be consistent among these
   views: ``check-names``, ``dnssec-accept-expired``,
   ``dnssec-validation``, ``max-cache-ttl``, ``max-ncache-ttl``,
   ``max-stale-ttl``, ``max-cache-size``, ``min-cache-ttl``,
   ``min-ncache-ttl``, and ``zero-no-soa-ttl``.

   Note that there may be other parameters that may cause confusion if
   they are inconsistent for different views that share a single cache.
   For example, if these views define different sets of forwarders that
   can return different answers for the same question, sharing the
   answer does not make sense or could even be harmful. It is the
   administrator's responsibility to ensure that configuration differences in
   different views do not cause disruption with a shared cache.

``directory``
   This sets the working directory of the server. Any non-absolute pathnames in
   the configuration file are taken as relative to this directory.
   The default location for most server output files (e.g.,
   ``named.run``) is this directory. If a directory is not specified,
   the working directory defaults to ``"."``, the directory from
   which the server was started. The directory specified should be an
   absolute path, and *must* be writable by the effective user ID of the
   ``named`` process.

   The option takes effect only at the time that the configuration
   option is parsed; if other files are being included before or after specifying the
   new ``directory``, the ``directory`` option must be listed
   before any other directive (like ``include``) that can work with relative
   files. The safest way to include files is to use absolute file names.

``dnstap``
   ``dnstap`` is a fast, flexible method for capturing and logging DNS
   traffic. Developed by Robert Edmonds at Farsight Security, Inc., and
   supported by multiple DNS implementations, ``dnstap`` uses
   ``libfstrm`` (a lightweight high-speed framing library; see
   https://github.com/farsightsec/fstrm) to send event payloads which
   are encoded using Protocol Buffers (``libprotobuf-c``, a mechanism
   for serializing structured data developed by Google, Inc.; see
   https://developers.google.com/protocol-buffers/).

   To enable ``dnstap`` at compile time, the ``fstrm`` and
   ``protobuf-c`` libraries must be available, and BIND must be
   configured with ``--enable-dnstap``.

   The ``dnstap`` option is a bracketed list of message types to be
   logged. These may be set differently for each view. Supported types
   are ``client``, ``auth``, ``resolver``, ``forwarder``, and
   ``update``. Specifying type ``all`` causes all ``dnstap``
   messages to be logged, regardless of type.

   Each type may take an additional argument to indicate whether to log
   ``query`` messages or ``response`` messages; if not specified, both
   queries and responses are logged.

   Example: To log all authoritative queries and responses, recursive
   client responses, and upstream queries sent by the resolver, use:

   ::

      dnstap {
        auth;
        client response;
        resolver query;
      };

   Logged ``dnstap`` messages can be parsed using the ``dnstap-read``
   utility (see :ref:`man_dnstap-read` for details).

   For more information on ``dnstap``, see http://dnstap.info.

   The fstrm library has a number of tunables that are exposed in
   ``named.conf``, and can be modified if necessary to improve
   performance or prevent loss of data. These are:

   -  ``fstrm-set-buffer-hint``: The threshold number of bytes to
      accumulate in the output buffer before forcing a buffer flush. The
      minimum is 1024, the maximum is 65536, and the default is 8192.

   -  ``fstrm-set-flush-timeout``: The number of seconds to allow
      unflushed data to remain in the output buffer. The minimum is 1
      second, the maximum is 600 seconds (10 minutes), and the default
      is 1 second.

   -  ``fstrm-set-output-notify-threshold``: The number of outstanding
      queue entries to allow on an input queue before waking the I/O
      thread. The minimum is 1 and the default is 32.

   -  ``fstrm-set-output-queue-model``: The queuing semantics
      to use for queue objects. The default is ``mpsc`` (multiple
      producer, single consumer); the other option is ``spsc`` (single
      producer, single consumer).

   -  ``fstrm-set-input-queue-size``: The number of queue entries to
      allocate for each input queue. This value must be a power of 2.
      The minimum is 2, the maximum is 16384, and the default is 512.

   -  ``fstrm-set-output-queue-size``: The number of queue entries to
      allocate for each output queue. The minimum is 2, the maximum is
      system-dependent and based on ``IOV_MAX``, and the default is 64.

   -  ``fstrm-set-reopen-interval``: The number of seconds to wait
      between attempts to reopen a closed output stream. The minimum is
      1 second, the maximum is 600 seconds (10 minutes), and the default
      is 5 seconds. For convenience, TTL-style time-unit suffixes may be
      used to specify the value.

   Note that all of the above minimum, maximum, and default values are
   set by the ``libfstrm`` library, and may be subject to change in
   future versions of the library. See the ``libfstrm`` documentation
   for more information.

``dnstap-output``
   This configures the path to which the ``dnstap`` frame stream is sent
   if ``dnstap`` is enabled at compile time and active.

   The first argument is either ``file`` or ``unix``, indicating whether
   the destination is a file or a Unix domain socket. The second
   argument is the path of the file or socket. (Note: when using a
   socket, ``dnstap`` messages are only sent if another process such
   as ``fstrm_capture`` (provided with ``libfstrm``) is listening on the
   socket.)

   If the first argument is ``file``, then up to three additional
   options can be added: ``size`` indicates the size to which a
   ``dnstap`` log file can grow before being rolled to a new file;
   ``versions`` specifies the number of rolled log files to retain; and
   ``suffix`` indicates whether to retain rolled log files with an
   incrementing counter as the suffix (``increment``) or with the
   current timestamp (``timestamp``). These are similar to the ``size``,
   ``versions``, and ``suffix`` options in a ``logging`` channel. The
   default is to allow ``dnstap`` log files to grow to any size without
   rolling.

   ``dnstap-output`` can only be set globally in ``options``. Currently,
   it can only be set once while ``named`` is running; once set, it
   cannot be changed by ``rndc reload`` or ``rndc reconfig``.

``dnstap-identity``
   This specifies an ``identity`` string to send in ``dnstap`` messages. If
   set to ``hostname``, which is the default, the server's hostname
   is sent. If set to ``none``, no identity string is sent.

``dnstap-version``
   This specifies a ``version`` string to send in ``dnstap`` messages. The
   default is the version number of the BIND release. If set to
   ``none``, no version string is sent.

``geoip-directory``
   When ``named`` is compiled using the MaxMind GeoIP2 geolocation API, this
   specifies the directory containing GeoIP database files.  By default, the
   option is set based on the prefix used to build the ``libmaxminddb`` module;
   for example, if the library is installed in ``/usr/local/lib``, then the
   default ``geoip-directory`` is ``/usr/local/share/GeoIP``. On Windows,
   the default is the ``named`` working directory.  See :ref:`acl`
   for details about ``geoip`` ACLs.

``key-directory``
   This is the directory where the public and private DNSSEC key files should be
   found when performing a dynamic update of secure zones, if different
   than the current working directory. (Note that this option has no
   effect on the paths for files containing non-DNSSEC keys such as
   ``bind.keys``, ``rndc.key``, or ``session.key``.)

``lmdb-mapsize``
   When ``named`` is built with liblmdb, this option sets a maximum size
   for the memory map of the new-zone database (NZD) in LMDB database
   format. This database is used to store configuration information for
   zones added using ``rndc addzone``. Note that this is not the NZD
   database file size, but the largest size that the database may grow
   to.

   Because the database file is memory-mapped, its size is limited by
   the address space of the ``named`` process. The default of 32 megabytes
   was chosen to be usable with 32-bit ``named`` builds. The largest
   permitted value is 1 terabyte. Given typical zone configurations
   without elaborate ACLs, a 32 MB NZD file ought to be able to hold
   configurations of about 100,000 zones.

``managed-keys-directory``
   This specifies the directory in which to store the files that track managed DNSSEC
   keys (i.e., those configured using the ``initial-key`` or ``initial-ds``
   keywords in a ``trust-anchors`` statement). By default, this is the working
   directory. The directory *must* be writable by the effective user ID of the
   ``named`` process.

   If ``named`` is not configured to use views, managed keys for
   the server are tracked in a single file called
   ``managed-keys.bind``. Otherwise, managed keys are tracked in
   separate files, one file per view; each file name is the view
   name (or, if it contains characters that are incompatible with use as
   a file name, the SHA256 hash of the view name), followed by the
   extension ``.mkeys``.

   (Note: in earlier releases, file names for views always used the
   SHA256 hash of the view name. To ensure compatibility after upgrading,
   if a file using the old name format is found to exist, it is
   used instead of the new format.)

``max-ixfr-ratio``
   This sets the size threshold (expressed as a percentage of the size
   of the full zone) beyond which ``named`` chooses to use an AXFR
   response rather than IXFR when answering zone transfer requests. See
   :ref:`incremental_zone_transfers`.

   The minimum value is ``1%``. The keyword ``unlimited`` disables ratio
   checking and allows IXFRs of any size. The default is ``unlimited``.

``new-zones-directory``
   This specifies the directory in which to store the configuration
   parameters for zones added via ``rndc addzone``. By default, this is
   the working directory. If set to a relative path, it is relative
   to the working directory. The directory *must* be writable by the
   effective user ID of the ``named`` process.

``qname-minimization``
   This option controls QNAME minimization behavior in the BIND
   resolver. When set to ``strict``, BIND follows the QNAME
   minimization algorithm to the letter, as specified in :rfc:`7816`.
   Setting this option to ``relaxed`` causes BIND to fall back to
   normal (non-minimized) query mode when it receives either NXDOMAIN or
   other unexpected responses (e.g., SERVFAIL, improper zone cut,
   REFUSED) to a minimized query. ``disabled`` disables QNAME
   minimization completely. The current default is ``relaxed``, but it
   may be changed to ``strict`` in a future release.

``tkey-gssapi-keytab``
   This is the KRB5 keytab file to use for GSS-TSIG updates. If this option is
   set and tkey-gssapi-credential is not set, updates are
   allowed with any key matching a principal in the specified keytab.

``tkey-gssapi-credential``
   This is the security credential with which the server should authenticate
   keys requested by the GSS-TSIG protocol. Currently only Kerberos 5
   authentication is available; the credential is a Kerberos
   principal which the server can acquire through the default system key
   file, normally ``/etc/krb5.keytab``. The location of the keytab file can be
   overridden using the ``tkey-gssapi-keytab`` option. Normally this
   principal is of the form ``DNS/server.domain``. To use
   GSS-TSIG, ``tkey-domain`` must also be set if a specific keytab is
   not set with ``tkey-gssapi-keytab``.

``tkey-domain``
   This domain is appended to the names of all shared keys generated with
   ``TKEY``. When a client requests a ``TKEY`` exchange, it may or may
   not specify the desired name for the key. If present, the name of the
   shared key is ``client-specified part`` + ``tkey-domain``.
   Otherwise, the name of the shared key is ``random hex digits``
   + ``tkey-domain``. In most cases, the ``domainname``
   should be the server's domain name, or an otherwise nonexistent
   subdomain like ``_tkey.domainname``. If using GSS-TSIG,
   this variable must be defined, unless a specific keytab
   is specified using ``tkey-gssapi-keytab``.

``tkey-dhkey``
   This is the Diffie-Hellman key used by the server to generate shared keys
   with clients using the Diffie-Hellman mode of ``TKEY``. The server
   must be able to load the public and private keys from files in the
   working directory. In most cases, the ``key_name`` should be the
   server's host name.

``cache-file``
   This is for testing only. Do not use.

``dump-file``
   This is the pathname of the file the server dumps the database to, when
   instructed to do so with ``rndc dumpdb``. If not specified, the
   default is ``named_dump.db``.

``memstatistics-file``
   This is the pathname of the file the server writes memory usage statistics to
   on exit. If not specified, the default is ``named.memstats``.

``lock-file``
   This is the pathname of a file on which ``named`` attempts to acquire a
   file lock when starting for the first time; if unsuccessful, the
   server terminates, under the assumption that another server
   is already running. If not specified, the default is
   ``none``.

   Specifying ``lock-file none`` disables the use of a lock file.
   ``lock-file`` is ignored if ``named`` was run using the ``-X``
   option, which overrides it. Changes to ``lock-file`` are ignored if
   ``named`` is being reloaded or reconfigured; it is only effective
   when the server is first started.

``pid-file``
   This is the pathname of the file the server writes its process ID in. If not
   specified, the default is ``/var/run/named/named.pid``. The PID file
   is used by programs that send signals to the running name
   server. Specifying ``pid-file none`` disables the use of a PID file;
   no file is written and any existing one is removed. Note
   that ``none`` is a keyword, not a filename, and therefore is not
   enclosed in double quotes.

``recursing-file``
   This is the pathname of the file where the server dumps the queries that are
   currently recursing, when instructed to do so with ``rndc recursing``.
   If not specified, the default is ``named.recursing``.

``statistics-file``
   This is the pathname of the file the server appends statistics to, when
   instructed to do so using ``rndc stats``. If not specified, the
   default is ``named.stats`` in the server's current directory. The
   format of the file is described in :ref:`statsfile`.

``bindkeys-file``
   This is the pathname of a file to override the built-in trusted keys provided
   by ``named``. See the discussion of ``dnssec-validation`` for
   details. If not specified, the default is ``/etc/bind.keys``.

``secroots-file``
   This is the pathname of the file the server dumps security roots to, when
   instructed to do so with ``rndc secroots``. If not specified, the
   default is ``named.secroots``.

``session-keyfile``
   This is the pathname of the file into which to write a TSIG session key
   generated by ``named`` for use by ``nsupdate -l``. If not specified,
   the default is ``/var/run/named/session.key``. (See :ref:`dynamic_update_policies`,
   and in particular the discussion of the ``update-policy`` statement's
   ``local`` option, for more information about this feature.)

``session-keyname``
   This is the key name to use for the TSIG session key. If not specified, the
   default is ``local-ddns``.

``session-keyalg``
   This is the algorithm to use for the TSIG session key. Valid values are
   hmac-sha1, hmac-sha224, hmac-sha256, hmac-sha384, hmac-sha512, and
   hmac-md5. If not specified, the default is hmac-sha256.

``port``
   This is the UDP/TCP port number the server uses to receive and send DNS
   protocol traffic. The default is 53. This option is mainly intended
   for server testing; a server using a port other than 53 is not
   able to communicate with the global DNS.

``dscp``
   This is the global Differentiated Services Code Point (DSCP) value to
   classify outgoing DNS traffic, on operating systems that support DSCP.
   Valid values are 0 through 63. It is not configured by default.

``random-device``
   This specifies a source of entropy to be used by the server; it is a
   device or file from which to read entropy. If it is a file,
   operations requiring entropy will fail when the file has been
   exhausted.

   Entropy is needed for cryptographic operations such as TKEY
   transactions, dynamic update of signed zones, and generation of TSIG
   session keys. It is also used for seeding and stirring the
   pseudo-random number generator which is used for less critical
   functions requiring randomness, such as generation of DNS message
   transaction IDs.

   If ``random-device`` is not specified, or if it is set to ``none``,
   entropy is read from the random number generation function
   supplied by the cryptographic library with which BIND was linked
   (i.e. OpenSSL or a PKCS#11 provider).

   The ``random-device`` option takes effect during the initial
   configuration load at server startup time and is ignored on
   subsequent reloads.

``preferred-glue``
   If specified, the listed type (A or AAAA) is emitted before
   other glue in the additional section of a query response. The default
   is to prefer A records when responding to queries that arrived via
   IPv4 and AAAA when responding to queries that arrived via IPv6.

.. _root-delegation-only:

``root-delegation-only``
   This turns on enforcement of delegation-only in TLDs (top-level domains)
   and root zones with an optional exclude list.

   DS queries are expected to be made to and be answered by delegation-only
   zones. Such queries and responses are treated as an exception to
   delegation-only processing and are not converted to NXDOMAIN
   responses, provided a CNAME is not discovered at the query name.

   If a delegation-only zone server also serves a child zone, it is not
   always possible to determine whether an answer comes from the
   delegation-only zone or the child zone. SOA NS and DNSKEY records are
   apex-only records and a matching response that contains these records
   or DS is treated as coming from a child zone. RRSIG records are also
   examined to see whether they are signed by a child zone, and the
   authority section is examined to see if there is evidence that
   the answer is from the child zone. Answers that are determined to be
   from a child zone are not converted to NXDOMAIN responses. Despite
   all these checks, there is still a possibility of false negatives when
   a child zone is being served.

   Similarly, false positives can arise from empty nodes (no records at
   the name) in the delegation-only zone when the query type is not ``ANY``.

   Note that some TLDs are not delegation-only; e.g., "DE", "LV", "US", and
   "MUSEUM". This list is not exhaustive.

   ::

      options {
          root-delegation-only exclude { "de"; "lv"; "us"; "museum"; };
      };

``disable-algorithms``
   This disables the specified DNSSEC algorithms at and below the specified
   name. Multiple ``disable-algorithms`` statements are allowed. Only
   the best-match ``disable-algorithms`` clause is used to
   determine the algorithms.

   If all supported algorithms are disabled, the zones covered by the
   ``disable-algorithms`` setting are treated as insecure.

   Configured trust anchors in ``trust-anchors`` (or ``managed-keys`` or
   ``trusted-keys``) that match a disabled algorithm are ignored and treated
   as if they were not configured.

``disable-ds-digests``
   This disables the specified DS digest types at and below the specified
   name. Multiple ``disable-ds-digests`` statements are allowed. Only
   the best-match ``disable-ds-digests`` clause is used to
   determine the digest types.

   If all supported digest types are disabled, the zones covered by
   ``disable-ds-digests`` are treated as insecure.

``dnssec-must-be-secure``
   This specifies hierarchies which must be or may not be secure (signed and
   validated). If ``yes``, then ``named`` only accepts answers if
   they are secure. If ``no``, then normal DNSSEC validation applies,
   allowing insecure answers to be accepted. The specified domain
   must be defined as a trust anchor, for instance in a ``trust-anchors``
   statement, or ``dnssec-validation auto`` must be active.

``dns64``
   This directive instructs ``named`` to return mapped IPv4 addresses to
   AAAA queries when there are no AAAA records. It is intended to be
   used in conjunction with a NAT64. Each ``dns64`` defines one DNS64
   prefix. Multiple DNS64 prefixes can be defined.

   Compatible IPv6 prefixes have lengths of 32, 40, 48, 56, 64, and 96, per
   :rfc:`6052`. Bits 64..71 inclusive must be zero, with the most significant bit
   of the prefix in position 0.

   In addition, a reverse IP6.ARPA zone is created for the prefix
   to provide a mapping from the IP6.ARPA names to the corresponding
   IN-ADDR.ARPA names using synthesized CNAMEs. ``dns64-server`` and
   ``dns64-contact`` can be used to specify the name of the server and
   contact for the zones. These can be set at the view/options
   level but not on a per-prefix basis.

   Each ``dns64`` supports an optional ``clients`` ACL that determines
   which clients are affected by this directive. If not defined, it
   defaults to ``any;``.

   Each ``dns64`` supports an optional ``mapped`` ACL that selects which
   IPv4 addresses are to be mapped in the corresponding A RRset. If not
   defined, it defaults to ``any;``.

   Normally, DNS64 does not apply to a domain name that owns one or more
   AAAA records; these records are simply returned. The optional
   ``exclude`` ACL allows specification of a list of IPv6 addresses that
   are ignored if they appear in a domain name's AAAA records;
   DNS64 is applied to any A records the domain name owns. If not
   defined, ``exclude`` defaults to ::ffff:0.0.0.0/96.

   An optional ``suffix`` can also be defined to set the bits trailing
   the mapped IPv4 address bits. By default these bits are set to
   ``::``. The bits matching the prefix and mapped IPv4 address must be
   zero.

   If ``recursive-only`` is set to ``yes``, the DNS64 synthesis only
   happens for recursive queries. The default is ``no``.

   If ``break-dnssec`` is set to ``yes``, the DNS64 synthesis happens
   even if the result, if validated, would cause a DNSSEC validation
   failure. If this option is set to ``no`` (the default), the DO is set
   on the incoming query, and there are RRSIGs on the applicable
   records, then synthesis does not happen.

   ::

          acl rfc1918 { 10/8; 192.168/16; 172.16/12; };

          dns64 64:FF9B::/96 {
              clients { any; };
              mapped { !rfc1918; any; };
              exclude { 64:FF9B::/96; ::ffff:0000:0000/96; };
              suffix ::;
          };

``dnssec-loadkeys-interval``
   When a zone is configured with ``auto-dnssec maintain;``, its key
   repository must be checked periodically to see if any new keys have
   been added or any existing keys' timing metadata has been updated
   (see :ref:`man_dnssec-keygen` and :ref:`man_dnssec-settime`).
   The ``dnssec-loadkeys-interval`` option
   sets the frequency of automatic repository checks, in minutes.  The
   default is ``60`` (1 hour), the minimum is ``1`` (1 minute), and
   the maximum is ``1440`` (24 hours); any higher value is silently
   reduced.

``dnssec-policy``
   This specifies which key and signing policy (KASP) should be used for this
   zone. This is a string referring to a ``dnssec-policy`` statement.  There
   are three built-in policies: ``default``, which uses the default policy,
   ``insecure``, to be used when you want to gracefully unsign your zone, and
   ``none``, which means no DNSSEC policy.  The default is ``none``.
   See :ref:`dnssec-policy Grammar <dnssec_policy_grammar>` for more details.

``dnssec-update-mode``
   If this option is set to its default value of ``maintain`` in a zone
   of type ``primary`` which is DNSSEC-signed and configured to allow
   dynamic updates (see :ref:`dynamic_update_policies`), and if ``named`` has access
   to the private signing key(s) for the zone, then ``named``
   automatically signs all new or changed records and maintains signatures
   for the zone by regenerating RRSIG records whenever they approach
   their expiration date.

   If the option is changed to ``no-resign``, then ``named`` signs
   all new or changed records, but scheduled maintenance of signatures
   is disabled.

   With either of these settings, ``named`` rejects updates to a
   DNSSEC-signed zone when the signing keys are inactive or unavailable
   to ``named``. (A planned third option, ``external``, will disable all
   automatic signing and allow DNSSEC data to be submitted into a zone
   via dynamic update; this is not yet implemented.)

``nta-lifetime``
   This specifies the default lifetime, in seconds, for
   negative trust anchors added via ``rndc nta``.

   A negative trust anchor selectively disables DNSSEC validation for
   zones that are known to be failing because of misconfiguration, rather
   than an attack. When data to be validated is at or below an active
   NTA (and above any other configured trust anchors), ``named``
   aborts the DNSSEC validation process and treats the data as insecure
   rather than bogus. This continues until the NTA's lifetime has
   elapsed. NTAs persist across ``named`` restarts.

   For convenience, TTL-style time-unit suffixes can be used to specify the NTA
   lifetime in seconds, minutes, or hours. It also accepts ISO 8601 duration
   formats.

   ``nta-lifetime`` defaults to one hour; it cannot exceed one week.

``nta-recheck``
   This specifies how often to check whether negative trust anchors added via
   ``rndc nta`` are still necessary.

   A negative trust anchor is normally used when a domain has stopped
   validating due to operator error; it temporarily disables DNSSEC
   validation for that domain. In the interest of ensuring that DNSSEC
   validation is turned back on as soon as possible, ``named``
   periodically sends a query to the domain, ignoring negative trust
   anchors, to find out whether it can now be validated. If so, the
   negative trust anchor is allowed to expire early.

   Validity checks can be disabled for an individual NTA by using
   ``rndc nta -f``, or for all NTAs by setting ``nta-recheck`` to zero.

   For convenience, TTL-style time-unit suffixes can be used to specify the NTA
   recheck interval in seconds, minutes, or hours. It also accepts ISO 8601
   duration formats.

   The default is five minutes. It cannot be longer than ``nta-lifetime``, which
   cannot be longer than a week.

``max-zone-ttl``

   This should now be configured as part of ``dnssec-policy``.
   Use of this option in ``options``, ``view`` and ``zone`` blocks has no
   effect on any zone for which a ``dnssec-policy`` has also been configured.

   ``max-zone-ttl`` specifies a maximum permissible TTL value in seconds.
   For convenience, TTL-style time-unit suffixes may be used to specify the
   maximum value. When a zone file is loaded, any record encountered with a
   TTL higher than ``max-zone-ttl`` causes the zone to be rejected.

   This is useful in DNSSEC-signed zones because when rolling to a new
   DNSKEY, the old key needs to remain available until RRSIG records
   have expired from caches. The ``max-zone-ttl`` option guarantees that
   the largest TTL in the zone is no higher than the set value.

   (Note: because ``map``-format files load directly into memory, this
   option cannot be used with them.)

   The default value is ``unlimited``. Setting ``max-zone-ttl`` to zero is
   equivalent to ``unlimited``.

``stale-answer-ttl``
   This specifies the TTL to be returned on stale answers. The default is 30
   seconds. The minimum allowed is 1 second; a value of 0 is updated silently
   to 1 second.

   For stale answers to be returned, they must be enabled, either in the
   configuration file using ``stale-answer-enable`` or via
   ``rndc serve-stale on``.

``serial-update-method``
   Zones configured for dynamic DNS may use this option to set the
   update method to be used for the zone serial number in the SOA
   record.

   With the default setting of ``serial-update-method increment;``, the
   SOA serial number is incremented by one each time the zone is
   updated.

   When set to ``serial-update-method unixtime;``, the SOA serial number
   is set to the number of seconds since the Unix epoch, unless the
   serial number is already greater than or equal to that value, in
   which case it is simply incremented by one.

   When set to ``serial-update-method date;``, the new SOA serial number
   is the current date in the form "YYYYMMDD", followed by two
   zeroes, unless the existing serial number is already greater than or
   equal to that value, in which case it is incremented by one.

``zone-statistics``
   If ``full``, the server collects statistical data on all zones,
   unless specifically turned off on a per-zone basis by specifying
   ``zone-statistics terse`` or ``zone-statistics none`` in the ``zone``
   statement. The statistical data includes, for example, DNSSEC signing
   operations and the number of authoritative answers per query type. The
   default is ``terse``, providing minimal statistics on zones
   (including name and current serial number, but not query type
   counters).

   These statistics may be accessed via the ``statistics-channel`` or
   using ``rndc stats``, which dumps them to the file listed in the
   ``statistics-file``. See also :ref:`statsfile`.

   For backward compatibility with earlier versions of BIND 9, the
   ``zone-statistics`` option can also accept ``yes`` or ``no``; ``yes``
   has the same meaning as ``full``. As of BIND 9.10, ``no`` has the
   same meaning as ``none``; previously, it was the same as ``terse``.

.. _boolean_options:

Boolean Options
^^^^^^^^^^^^^^^

``automatic-interface-scan``
   If ``yes`` and supported by the operating system, this automatically rescans
   network interfaces when the interface addresses are added or removed.  The
   default is ``yes``.  This configuration option does not affect the time-based
   ``interface-interval`` option; it is recommended to set the time-based
   ``interface-interval`` to 0 when the operator confirms that automatic
   interface scanning is supported by the operating system.

   The ``automatic-interface-scan`` implementation uses routing sockets for the
   network interface discovery; therefore, the operating system must
   support the routing sockets for this feature to work.

``allow-new-zones``
   If ``yes``, then zones can be added at runtime via ``rndc addzone``.
   The default is ``no``.

   Newly added zones' configuration parameters are stored so that they
   can persist after the server is restarted. The configuration
   information is saved in a file called ``viewname.nzf`` (or, if
   ``named`` is compiled with liblmdb, in an LMDB database file called
   ``viewname.nzd``). "viewname" is the name of the view, unless the view
   name contains characters that are incompatible with use as a file
   name, in which case a cryptographic hash of the view name is used
   instead.

   Configurations for zones added at runtime are stored either in
   a new-zone file (NZF) or a new-zone database (NZD), depending on
   whether ``named`` was linked with liblmdb at compile time. See
   :ref:`man_rndc` for further details about ``rndc addzone``.

``auth-nxdomain``
   If ``yes``, then the ``AA`` bit is always set on NXDOMAIN responses,
   even if the server is not actually authoritative. The default is
   ``no``.

``memstatistics``
   This writes memory statistics to the file specified by
   ``memstatistics-file`` at exit. The default is ``no`` unless ``-m
   record`` is specified on the command line, in which case it is ``yes``.

``dialup``
   If ``yes``, then the server treats all zones as if they are doing
   zone transfers across a dial-on-demand dialup link, which can be
   brought up by traffic originating from this server. Although this setting has
   different effects according to zone type, it concentrates the zone
   maintenance so that everything happens quickly, once every
   ``heartbeat-interval``, ideally during a single call. It also
   suppresses some normal zone maintenance traffic. The default
   is ``no``.

   If specified in the ``view`` and
   ``zone`` statements, the ``dialup`` option overrides the global ``dialup``
   option.

   If the zone is a primary zone, the server sends out a NOTIFY
   request to all the secondaries (default). This should trigger the zone
   serial number check in the secondary (providing it supports NOTIFY),
   allowing the secondary to verify the zone while the connection is active.
   The set of servers to which NOTIFY is sent can be controlled by
   ``notify`` and ``also-notify``.

   If the zone is a secondary or stub zone, the server suppresses
   the regular "zone up to date" (refresh) queries and only performs them
   when the ``heartbeat-interval`` expires, in addition to sending NOTIFY
   requests.

   Finer control can be achieved by using ``notify``, which only sends
   NOTIFY messages; ``notify-passive``, which sends NOTIFY messages and
   suppresses the normal refresh queries; ``refresh``, which suppresses
   normal refresh processing and sends refresh queries when the
   ``heartbeat-interval`` expires; and ``passive``, which disables
   normal refresh processing.

   +--------------------+-----------------+-----------------+-----------------+
   | dialup mode        | normal refresh  | heart-beat      | heart-beat      |
   |                    |                 | refresh         | notify          |
   +--------------------+-----------------+-----------------+-----------------+
   | ``no``             | yes             | no              | no              |
   | (default)          |                 |                 |                 |
   +--------------------+-----------------+-----------------+-----------------+
   | ``yes``            | no              | yes             | yes             |
   +--------------------+-----------------+-----------------+-----------------+
   | ``notify``         | yes             | no              | yes             |
   +--------------------+-----------------+-----------------+-----------------+
   | ``refresh``        | no              | yes             | no              |
   +--------------------+-----------------+-----------------+-----------------+
   | ``passive``        | no              | no              | no              |
   +--------------------+-----------------+-----------------+-----------------+
   | ``notify-passive`` | no              | no              | yes             |
   +--------------------+-----------------+-----------------+-----------------+

   Note that normal NOTIFY processing is not affected by ``dialup``.

``flush-zones-on-shutdown``
   When the name server exits upon receiving SIGTERM, flush or do not
   flush any pending zone writes. The default is
   ``flush-zones-on-shutdown no``.

``geoip-use-ecs``
   This option was part of an experimental implementation of the EDNS
   CLIENT-SUBNET for authoritative servers, but is now obsolete.

``root-key-sentinel``
   If ``yes``, respond to root key sentinel probes as described in
   draft-ietf-dnsop-kskroll-sentinel-08. The default is ``yes``.

``reuseport``
   This option enables kernel load-balancing of sockets on systems which support
   it, including Linux (SO_REUSEPORT) and FreeBSD (SO_REUSEPORT_LB). This
   instructs the kernel to distribute incoming socket connections among the
   networking threads based on a hashing scheme. For more information, see the
   receive network flow classification options (``rx-flow-hash``) section in the
   ``ethtool`` manual page. The default is ``yes``.

   Enabling ``reuseport`` significantly increases general throughput when
   incoming traffic is distributed uniformly onto the threads by the
   operating system. However, in cases where a worker thread is busy with a
   long-lasting operation, such as processing a Response Policy Zone (RPZ) or
   Catalog Zone update or an unusually large zone transfer, incoming traffic
   that hashes onto that thread may be delayed. On servers where these events
   occur frequently, it may be preferable to disable socket load-balancing so
   that other threads can pick up the traffic that would have been sent to the
   busy thread.

   Note: this option can only be set when ``named`` first starts.
   Changes will not take effect during reconfiguration; the server
   must be restarted.

``message-compression``
   If ``yes``, DNS name compression is used in responses to regular
   queries (not including AXFR or IXFR, which always use compression).
   Setting this option to ``no`` reduces CPU usage on servers and may
   improve throughput. However, it increases response size, which may
   cause more queries to be processed using TCP; a server with
   compression disabled is out of compliance with :rfc:`1123` Section
   6.1.3.2. The default is ``yes``.

``minimal-responses``
   This option controls the addition of records to the authority and
   additional sections of responses. Such records may be included in
   responses to be helpful to clients; for example, MX records may
   have associated address records included in the additional section,
   obviating the need for a separate address lookup. However, adding
   these records to responses is not mandatory and requires additional
   database lookups, causing extra latency when marshalling responses.

   Responses to DNSKEY, DS, CDNSKEY, and CDS requests will never have
   optional additional records added. Responses to NS requests will
   always have additional section processing.

   ``minimal-responses`` takes one of four values:

   -  ``no``: the server is as complete as possible when generating
      responses.
   -  ``yes``: the server only adds records to the authority and additional
      sections when such records are required by the DNS protocol (for
      example, when returning delegations or negative responses). This
      provides the best server performance but may result in more client
      queries.
   -  ``no-auth``: the server omits records from the authority section except
      when they are required, but it may still add records to the
      additional section.
   -  ``no-auth-recursive``: the same as ``no-auth`` when recursion is requested
      in the query (RD=1), or the same as ``no`` if recursion is not requested.

   ``no-auth`` and ``no-auth-recursive`` are useful when answering stub
   clients, which usually ignore the authority section.
   ``no-auth-recursive`` is meant for use in mixed-mode servers that
   handle both authoritative and recursive queries.

   The default is ``no-auth-recursive``.

``glue-cache``
   When set to ``yes``, a cache is used to improve query performance
   when adding address-type (A and AAAA) glue records to the additional
   section of DNS response messages that delegate to a child zone.

   The glue cache uses memory proportional to the number of delegations
   in the zone. The default setting is ``yes``, which improves
   performance at the cost of increased memory usage for the zone. To avoid
   this, set it to ``no``.

``minimal-any``
   If set to ``yes``, the server replies with only one of
   the RRsets for the query name, and its covering RRSIGs if any,
   when generating a positive response to a query of type ANY over UDP,
   instead of replying with all known RRsets for the name. Similarly, a
   query for type RRSIG is answered with the RRSIG records covering
   only one type. This can reduce the impact of some kinds of attack
   traffic, without harming legitimate clients. (Note, however, that the
   RRset returned is the first one found in the database; it is not
   necessarily the smallest available RRset.) Additionally,
   ``minimal-responses`` is turned on for these queries, so no
   unnecessary records are added to the authority or additional
   sections. The default is ``no``.

``notify``
   If set to ``yes`` (the default), DNS NOTIFY messages are sent when a
   zone the server is authoritative for changes; see :ref:`notify`.
   The messages are sent to the servers listed in the zone's NS records
   (except the primary server identified in the SOA MNAME field), and to
   any servers listed in the ``also-notify`` option.

   If set to ``primary-only`` (or the older keyword ``master-only``),
   notifies are only sent for primary zones. If set to ``explicit``,
   notifies are sent only to servers explicitly listed using
   ``also-notify``. If set to ``no``, no notifies are sent.

   The ``notify`` option may also be specified in the ``zone``
   statement, in which case it overrides the ``options notify``
   statement. It would only be necessary to turn off this option if it
   caused secondary zones to crash.

``notify-to-soa``
   If ``yes``, do not check the name servers in the NS RRset against the
   SOA MNAME. Normally a NOTIFY message is not sent to the SOA MNAME
   (SOA ORIGIN), as it is supposed to contain the name of the ultimate
   primary server. Sometimes, however, a secondary server is listed as the SOA MNAME in
   hidden primary configurations; in that case, the
   ultimate primary should be set to still send NOTIFY messages to all the name servers
   listed in the NS RRset.

``recursion``
   If ``yes``, and a DNS query requests recursion, then the server
   attempts to do all the work required to answer the query. If recursion
   is off and the server does not already know the answer, it
   returns a referral response. The default is ``yes``. Note that setting
   ``recursion no`` does not prevent clients from getting data from the
   server's cache; it only prevents new data from being cached as an
   effect of client queries. Caching may still occur as an effect of the
   server's internal operation, such as NOTIFY address lookups.

``request-nsid``
   If ``yes``, then an empty EDNS(0) NSID (Name Server Identifier)
   option is sent with all queries to authoritative name servers during
   iterative resolution. If the authoritative server returns an NSID
   option in its response, then its contents are logged in the ``nsid``
   category at level ``info``. The default is ``no``.

``request-sit``
   This experimental option is obsolete.

``require-server-cookie``
   If ``yes``, require a valid server cookie before sending a full response to a UDP
   request from a cookie-aware client. BADCOOKIE is sent if there is a
   bad or nonexistent server cookie.

   The default is ``no``.

   Users wishing to test that DNS COOKIE clients correctly handle
   BADCOOKIE, or who are getting a lot of forged DNS requests with DNS COOKIES
   present, should set this to ``yes``. Setting this to ``yes`` results in a reduced amplification effect
   in a reflection attack, as the BADCOOKIE response is smaller than a full
   response, while also requiring a legitimate client to follow up with a second
   query with the new, valid, cookie.

``answer-cookie``
   When set to the default value of ``yes``, COOKIE EDNS options are
   sent when applicable in replies to client queries. If set to ``no``,
   COOKIE EDNS options are not sent in replies. This can only be set
   at the global options level, not per-view.

   ``answer-cookie no`` is intended as a temporary measure, for use when
   ``named`` shares an IP address with other servers that do not yet
   support DNS COOKIE. A mismatch between servers on the same address is
   not expected to cause operational problems, but the option to disable
   COOKIE responses so that all servers have the same behavior is
   provided out of an abundance of caution. DNS COOKIE is an important
   security mechanism, and should not be disabled unless absolutely
   necessary.

``send-cookie``
   If ``yes``, then a COOKIE EDNS option is sent along with the query.
   If the resolver has previously communicated with the server, the COOKIE
   returned in the previous transaction is sent. This is used by the
   server to determine whether the resolver has talked to it before. A
   resolver sending the correct COOKIE is assumed not to be an off-path
   attacker sending a spoofed-source query; the query is therefore
   unlikely to be part of a reflection/amplification attack, so
   resolvers sending a correct COOKIE option are not subject to response
   rate limiting (RRL). Resolvers which do not send a correct COOKIE
   option may be limited to receiving smaller responses via the
   ``nocookie-udp-size`` option.

   The default is ``yes``.

``stale-answer-enable``
   If ``yes``, enable the returning of "stale" cached answers when the name
   servers for a zone are not answering and the ``stale-cache-enable`` option is
   also enabled. The default is not to return stale answers.

   Stale answers can also be enabled or disabled at runtime via
   ``rndc serve-stale on`` or ``rndc serve-stale off``; these override
   the configured setting. ``rndc serve-stale reset`` restores the
   setting to the one specified in ``named.conf``. Note that if stale
   answers have been disabled by ``rndc``, they cannot be
   re-enabled by reloading or reconfiguring ``named``; they must be
   re-enabled with ``rndc serve-stale on``, or the server must be
   restarted.

   Information about stale answers is logged under the ``serve-stale``
   log category.

``stale-answer-client-timeout``
   This option defines the amount of time (in milliseconds) that ``named``
   waits before attempting to answer the query with a stale RRset from cache.
   If a stale answer is found, ``named`` continues the ongoing fetches,
   attempting to refresh the RRset in cache until the
   ``resolver-query-timeout`` interval is reached.

   This option is off by default, which is equivalent to setting it to
   ``off`` or ``disabled``. It also has no effect if ``stale-answer-enable``
   is disabled.

   The maximum value for this option is ``resolver-query-timeout`` minus
   one second. The minimum value, ``0``, causes a cached (stale) RRset to be
   immediately returned if it is available while still attempting to
   refresh the data in cache. :rfc:`8767` recommends a value of ``1800``
   (milliseconds).

``stale-cache-enable``
   If ``yes``, enable the retaining of "stale" cached answers.  Default ``yes``.

``stale-refresh-time``
   If the name servers for a given zone are not answering, this sets the time
   window for which ``named`` will promptly return "stale" cached answers for
   that RRSet being requested before a new attempt in contacting the servers
   is made. For convenience, TTL-style time-unit suffixes may be used to
   specify the value. It also accepts ISO 8601 duration formats.

   The default ``stale-refresh-time`` is 30 seconds, as :rfc:`8767` recommends
   that attempts to refresh to be done no more frequently than every 30
   seconds. A value of zero disables the feature, meaning that normal
   resolution will take place first, if that fails only then ``named`` will
   return "stale" cached answers.

``nocookie-udp-size``
   This sets the maximum size of UDP responses that are sent to queries
   without a valid server COOKIE. A value below 128 is silently
   raised to 128. The default value is 4096, but the ``max-udp-size``
   option may further limit the response size as the default for
   ``max-udp-size`` is 4096.

``sit-secret``
   This experimental option is obsolete.

``cookie-algorithm``
   This sets the algorithm to be used when generating the server cookie; the options are
   "aes" or "siphash24". The default is "siphash24". The "aes" option remains for legacy
   purposes.

``cookie-secret``
   If set, this is a shared secret used for generating and verifying
   EDNS COOKIE options within an anycast cluster. If not set, the system
   generates a random secret at startup. The shared secret is
   encoded as a hex string and needs to be 128 bits for either "siphash24"
   or "aes".

   If there are multiple secrets specified, the first one listed in
   ``named.conf`` is used to generate new server cookies. The others
   are only used to verify returned cookies.

``response-padding``
   The EDNS Padding option is intended to improve confidentiality when
   DNS queries are sent over an encrypted channel, by reducing the
   variability in packet sizes. If a query:

   1. contains an EDNS Padding option,
   2. includes a valid server cookie or uses TCP,
   3. is not signed using TSIG or SIG(0), and
   4. is from a client whose address matches the specified ACL,

   then the response is padded with an EDNS Padding option to a multiple
   of ``block-size`` bytes. If these conditions are not met, the
   response is not padded.

   If ``block-size`` is 0 or the ACL is ``none;``, this feature is
   disabled and no padding occurs; this is the default. If
   ``block-size`` is greater than 512, a warning is logged and the value
   is truncated to 512. Block sizes are ordinarily expected to be powers
   of two (for instance, 128), but this is not mandatory.

``trust-anchor-telemetry``
   This causes ``named`` to send specially formed queries once per day to
   domains for which trust anchors have been configured via, e.g.,
   ``trust-anchors`` or ``dnssec-validation auto``.

   The query name used for these queries has the form
   ``_ta-xxxx(-xxxx)(...).<domain>``, where each "xxxx" is a group of four
   hexadecimal digits representing the key ID of a trusted DNSSEC key.
   The key IDs for each domain are sorted smallest to largest prior to
   encoding. The query type is NULL.

   By monitoring these queries, zone operators are able to see which
   resolvers have been updated to trust a new key; this may help them
   decide when it is safe to remove an old one.

   The default is ``yes``.

``use-ixfr``
   *This option is obsolete*. To disable IXFR to a
   particular server or servers, see the information on the
   ``provide-ixfr`` option in :ref:`server_statement_definition_and_usage`.
   See also :ref:`incremental_zone_transfers`.

``provide-ixfr``
   See the description of ``provide-ixfr`` in :ref:`server_statement_definition_and_usage`.

``request-ixfr``
   See the description of ``request-ixfr`` in :ref:`server_statement_definition_and_usage`.

``request-expire``
   See the description of ``request-expire`` in :ref:`server_statement_definition_and_usage`.

``match-mapped-addresses``
   If ``yes``, then an IPv4-mapped IPv6 address matches any
   address-match list entries that match the corresponding IPv4 address.

   This option was introduced to work around a kernel quirk in some
   operating systems that causes IPv4 TCP connections, such as zone
   transfers, to be accepted on an IPv6 socket using mapped addresses.
   This caused address-match lists designed for IPv4 to fail to match.
   However, ``named`` now solves this problem internally. The use of
   this option is discouraged.

``ixfr-from-differences``
   When ``yes`` and the server loads a new version of a primary zone from
   its zone file or receives a new version of a secondary file via zone
   transfer, it compares the new version to the previous one and
   calculates a set of differences. The differences are then logged in
   the zone's journal file so that the changes can be transmitted to
   downstream secondaries as an incremental zone transfer.

   By allowing incremental zone transfers to be used for non-dynamic
   zones, this option saves bandwidth at the expense of increased CPU
   and memory consumption at the primary server. In particular, if the new
   version of a zone is completely different from the previous one, the
   set of differences is of a size comparable to the combined size
   of the old and new zone versions, and the server needs to
   temporarily allocate memory to hold this complete difference set.

   ``ixfr-from-differences`` also accepts ``primary``
   and ``secondary`` at the view and options levels,
   which causes ``ixfr-from-differences`` to be enabled for all primary
   or secondary zones, respectively. It is off for all zones by default.

   Note: if inline signing is enabled for a zone, the user-provided
   ``ixfr-from-differences`` setting is ignored for that zone.

``multi-master``
   This should be set when there are multiple primary servers for a zone and the
   addresses refer to different machines. If ``yes``, ``named`` does not
   log when the serial number on the primary is less than what ``named``
   currently has. The default is ``no``.

``auto-dnssec``
   Zones configured for dynamic DNS may use this option to allow varying
   levels of automatic DNSSEC key management. There are three possible
   settings:

   ``auto-dnssec allow;`` permits keys to be updated and the zone fully
   re-signed whenever the user issues the command ``rndc sign zonename``.

   ``auto-dnssec maintain;`` includes the above, but also
   automatically adjusts the zone's DNSSEC keys on a schedule, according
   to the keys' timing metadata (see :ref:`man_dnssec-keygen` and
   :ref:`man_dnssec-settime`). The command ``rndc sign zonename``
   causes ``named`` to load keys from the key repository and sign the
   zone with all keys that are active. ``rndc loadkeys zonename``
   causes ``named`` to load keys from the key repository and schedule
   key maintenance events to occur in the future, but it does not sign
   the full zone immediately. Note: once keys have been loaded for a
   zone the first time, the repository is searched for changes
   periodically, regardless of whether ``rndc loadkeys`` is used. The
   recheck interval is defined by ``dnssec-loadkeys-interval``.

   ``auto-dnssec off;`` does not allow for DNSSEC key management.
   This is the default setting.

   This option may only be activated at the zone level; if configured
   at the view or options level, it must be set to ``off``.

   The DNSSEC records are written to the zone's filename set in ``file``,
   unless ``inline-signing`` is enabled.

``dnssec-enable``
   This option is obsolete and has no effect.

.. _dnssec-validation-option:

``dnssec-validation``
   This option enables DNSSEC validation in ``named``.

   If set to ``auto``, DNSSEC validation is enabled and a default trust
   anchor for the DNS root zone is used. This trust anchor is provided
   as part of BIND and is kept up-to-date using :ref:`rfc5011.support` key
   management.

   If set to ``yes``, DNSSEC validation is enabled, but a trust anchor must be
   manually configured using a ``trust-anchors`` statement (or the
   ``managed-keys`` or ``trusted-keys`` statements, both deprecated). If
   there is no configured trust anchor, validation does not take place.

   If set to ``no``, DNSSEC validation is disabled.

   The default is ``auto``, unless BIND is built with
   ``configure --disable-auto-validation``, in which case the default is
   ``yes``.

   The default root trust anchor is stored in the file ``bind.keys``.
   ``named`` loads that key at startup if ``dnssec-validation`` is
   set to ``auto``. A copy of the file is installed along with BIND 9,
   and is current as of the release date. If the root key expires, a new
   copy of ``bind.keys`` can be downloaded from
   https://www.isc.org/bind-keys.

   (To prevent problems if ``bind.keys`` is not found, the current trust
   anchor is also compiled in ``named``. Relying on this is not
   recommended, however, as it requires ``named`` to be recompiled with
   a new key when the root key expires.)

   .. note:: ``named`` loads *only* the root key from ``bind.keys``. The file
         cannot be used to store keys for other zones. The root key in
         ``bind.keys`` is ignored if ``dnssec-validation auto`` is not in
         use.

         Whenever the resolver sends out queries to an EDNS-compliant
         server, it always sets the DO bit indicating it can support DNSSEC
         responses, even if ``dnssec-validation`` is off.

``validate-except``
   This specifies a list of domain names at and beneath which DNSSEC
   validation should *not* be performed, regardless of the presence of a
   trust anchor at or above those names. This may be used, for example,
   when configuring a top-level domain intended only for local use, so
   that the lack of a secure delegation for that domain in the root zone
   does not cause validation failures. (This is similar to setting a
   negative trust anchor except that it is a permanent configuration,
   whereas negative trust anchors expire and are removed after a set
   period of time.)

``dnssec-accept-expired``
   This accepts expired signatures when verifying DNSSEC signatures. The
   default is ``no``. Setting this option to ``yes`` leaves ``named``
   vulnerable to replay attacks.

``querylog``
   Query logging provides a complete log of all incoming queries and all query
   errors. This provides more insight into the server's activity, but with a
   cost to performance which may be significant on heavily loaded servers.

   The ``querylog`` option specifies whether query logging should be active when
   ``named`` first starts.  If ``querylog`` is not specified, then query logging
   is determined by the presence of the logging category ``queries``.  Query
   logging can also be activated at runtime using the command ``rndc querylog
   on``, or deactivated with ``rndc querylog off``.

``check-names``
   This option is used to restrict the character set and syntax of
   certain domain names in primary files and/or DNS responses received
   from the network. The default varies according to usage area. For
   ``primary`` zones the default is ``fail``. For ``secondary`` zones the
   default is ``warn``. For answers received from the network
   (``response``), the default is ``ignore``.

   The rules for legal hostnames and mail domains are derived from
   :rfc:`952` and :rfc:`821` as modified by :rfc:`1123`.

   ``check-names`` applies to the owner names of A, AAAA, and MX records.
   It also applies to the domain names in the RDATA of NS, SOA, MX, and
   SRV records. It further applies to the RDATA of PTR records where the
   owner name indicates that it is a reverse lookup of a hostname (the
   owner name ends in IN-ADDR.ARPA, IP6.ARPA, or IP6.INT).

``check-dup-records``
   This checks primary zones for records that are treated as different by
   DNSSEC but are semantically equal in plain DNS. The default is to
   ``warn``. Other possible values are ``fail`` and ``ignore``.

``check-mx``
   This checks whether the MX record appears to refer to an IP address. The
   default is to ``warn``. Other possible values are ``fail`` and
   ``ignore``.

``check-wildcard``
   This option is used to check for non-terminal wildcards. The use of
   non-terminal wildcards is almost always as a result of a lack of
   understanding of the wildcard matching algorithm (:rfc:`1034`). This option
   affects primary zones. The default (``yes``) is to check for
   non-terminal wildcards and issue a warning.

``check-integrity``
   This performs post-load zone integrity checks on primary zones. It checks
   that MX and SRV records refer to address (A or AAAA) records and that
   glue address records exist for delegated zones. For MX and SRV
   records, only in-zone hostnames are checked (for out-of-zone hostnames,
   use ``named-checkzone``). For NS records, only names below top-of-zone
   are checked (for out-of-zone names and glue consistency checks, use
   ``named-checkzone``). The default is ``yes``.

   The use of the SPF record to publish Sender Policy Framework is
   deprecated, as the migration from using TXT records to SPF records was
   abandoned. Enabling this option also checks that a TXT Sender Policy
   Framework record exists (starts with "v=spf1") if there is an SPF
   record. Warnings are emitted if the TXT record does not exist; they can
   be suppressed with ``check-spf``.

``check-mx-cname``
   If ``check-integrity`` is set, then fail, warn, or ignore MX records
   that refer to CNAMES. The default is to ``warn``.

``check-srv-cname``
   If ``check-integrity`` is set, then fail, warn, or ignore SRV records
   that refer to CNAMES. The default is to ``warn``.

``check-sibling``
   When performing integrity checks, also check that sibling glue
   exists. The default is ``yes``.

``check-spf``
   If ``check-integrity`` is set, check that there is a TXT Sender
   Policy Framework record present (starts with "v=spf1") if there is an
   SPF record present. The default is ``warn``.

``zero-no-soa-ttl``
   If ``yes``, when returning authoritative negative responses to SOA queries, set
   the TTL of the SOA record returned in the authority section to zero.
   The default is ``yes``.

``zero-no-soa-ttl-cache``
   If ``yes``, when caching a negative response to an SOA query set the TTL to zero.
   The default is ``no``.

``update-check-ksk``
   When set to the default value of ``yes``, check the KSK bit in each
   key to determine how the key should be used when generating RRSIGs
   for a secure zone.

   Ordinarily, zone-signing keys (that is, keys without the KSK bit set)
   are used to sign the entire zone, while key-signing keys (keys with
   the KSK bit set) are only used to sign the DNSKEY RRset at the zone
   apex. However, if this option is set to ``no``, then the KSK bit is
   ignored; KSKs are treated as if they were ZSKs and are used to sign
   the entire zone. This is similar to the ``dnssec-signzone -z``
   command-line option.

   When this option is set to ``yes``, there must be at least two active
   keys for every algorithm represented in the DNSKEY RRset: at least
   one KSK and one ZSK per algorithm. If there is any algorithm for
   which this requirement is not met, this option is ignored for
   that algorithm.

``dnssec-dnskey-kskonly``
   When this option and ``update-check-ksk`` are both set to ``yes``,
   only key-signing keys (that is, keys with the KSK bit set) are
   used to sign the DNSKEY, CDNSKEY, and CDS RRsets at the zone apex.
   Zone-signing keys (keys without the KSK bit set) are used to sign
   the remainder of the zone, but not the DNSKEY RRset. This is similar
   to the ``dnssec-signzone -x`` command-line option.

   The default is ``no``. If ``update-check-ksk`` is set to ``no``, this
   option is ignored.

``try-tcp-refresh``
   If ``yes``, try to refresh the zone using TCP if UDP queries fail. The default is
   ``yes``.

``dnssec-secure-to-insecure``
   This allows a dynamic zone to transition from secure to insecure (i.e.,
   signed to unsigned) by deleting all of the DNSKEY records. The
   default is ``no``. If set to ``yes``, and if the DNSKEY RRset at the
   zone apex is deleted, all RRSIG and NSEC records are removed from
   the zone as well.

   If the zone uses NSEC3, it is also necessary to delete the
   NSEC3PARAM RRset from the zone apex; this causes the removal of
   all corresponding NSEC3 records. (It is expected that this
   requirement will be eliminated in a future release.)

   Note that if a zone has been configured with ``auto-dnssec maintain``
   and the private keys remain accessible in the key repository,
   the zone will be automatically signed again the next time ``named``
   is started.

``synth-from-dnssec``
   This option synthesizes answers from cached NSEC, NSEC3, and other RRsets that have been
   proved to be correct using DNSSEC. The default is ``no``, but it will become
   ``yes`` again in future releases.

   .. note:: DNSSEC validation must be enabled for this option to be effective.
      This initial implementation only covers synthesis of answers from
      NSEC records; synthesis from NSEC3 is planned for the future. This
      will also be controlled by ``synth-from-dnssec``.

Forwarding
^^^^^^^^^^

The forwarding facility can be used to create a large site-wide cache on
a few servers, reducing traffic over links to external name servers. It
can also be used to allow queries by servers that do not have direct
access to the Internet, but wish to look up exterior names anyway.
Forwarding occurs only on those queries for which the server is not
authoritative and does not have the answer in its cache.

``forward``
   This option is only meaningful if the forwarders list is not empty. A
   value of ``first`` is the default and causes the server to query the
   forwarders first; if that does not answer the question, the
   server then looks for the answer itself. If ``only`` is
   specified, the server only queries the forwarders.

``forwarders``
   This specifies a list of IP addresses to which queries are forwarded. The
   default is the empty list (no forwarding). Each address in the list can be
   associated with an optional port number and/or DSCP value, and a default port
   number and DSCP value can be set for the entire list.

Forwarding can also be configured on a per-domain basis, allowing for
the global forwarding options to be overridden in a variety of ways.
Particular domains can be set to use different forwarders, or have a
different ``forward only/first`` behavior, or not forward at all; see
:ref:`zone_statement_grammar`.

.. _dual_stack:

Dual-stack Servers
^^^^^^^^^^^^^^^^^^

Dual-stack servers are used as servers of last resort, to work around
problems in reachability due to the lack of support for either IPv4 or IPv6
on the host machine.

``dual-stack-servers``
   This specifies host names or addresses of machines with access to both
   IPv4 and IPv6 transports. If a hostname is used, the server must be
   able to resolve the name using only the transport it has. If the
   machine is dual-stacked, the ``dual-stack-servers`` parameter has no
   effect unless access to a transport has been disabled on the command
   line (e.g., ``named -4``).

.. _access_control:

Access Control
^^^^^^^^^^^^^^

Access to the server can be restricted based on the IP address of the
requesting system. See :ref:`address_match_lists`
for details on how to specify IP address lists.

``allow-notify``
   This ACL specifies which hosts may send NOTIFY messages to inform
   this server of changes to zones for which it is acting as a secondary
   server. This is only applicable for secondary zones (i.e., type
   ``secondary`` or ``slave``).

   If this option is set in ``view`` or ``options``, it is globally
   applied to all secondary zones. If set in the ``zone`` statement, the
   global value is overridden.

   If not specified, the default is to process NOTIFY messages only from
   the configured ``primaries`` for the zone. ``allow-notify`` can be used
   to expand the list of permitted hosts, not to reduce it.

``allow-query``
   This specifies which hosts are allowed to ask ordinary DNS questions.
   ``allow-query`` may also be specified in the ``zone`` statement, in
   which case it overrides the ``options allow-query`` statement. If not
   specified, the default is to allow queries from all hosts.

   .. note:: ``allow-query-cache`` is used to specify access to the cache.

``allow-query-on``
   This specifies which local addresses can accept ordinary DNS questions.
   This makes it possible, for instance, to allow queries on
   internal-facing interfaces but disallow them on external-facing ones,
   without necessarily knowing the internal network's addresses.

   Note that ``allow-query-on`` is only checked for queries that are
   permitted by ``allow-query``. A query must be allowed by both ACLs,
   or it is refused.

   ``allow-query-on`` may also be specified in the ``zone`` statement,
   in which case it overrides the ``options allow-query-on`` statement.

   If not specified, the default is to allow queries on all addresses.

   .. note:: ``allow-query-cache`` is used to specify access to the cache.

``allow-query-cache``
   This specifies which hosts are allowed to get answers from the cache. If
   ``allow-recursion`` is not set, BIND checks to see if the following parameters
   are set, in order: ``allow-query-cache`` and ``allow-query`` (unless ``recursion no;`` is set).
   If neither of those parameters is set, the default (localnets; localhost;) is used.

``allow-query-cache-on``
   This specifies which local addresses can send answers from the cache. If
   ``allow-query-cache-on`` is not set, then ``allow-recursion-on`` is
   used if set. Otherwise, the default is to allow cache responses to be
   sent from any address. Note: both ``allow-query-cache`` and
   ``allow-query-cache-on`` must be satisfied before a cache response
   can be sent; a client that is blocked by one cannot be allowed by the
   other.

``allow-recursion``
   This specifies which hosts are allowed to make recursive queries through
   this server. BIND checks to see if the following parameters are set, in
   order: ``allow-query-cache`` and ``allow-query``. If neither of those parameters
   is set, the default (localnets; localhost;) is used.

``allow-recursion-on``
   This specifies which local addresses can accept recursive queries. If
   ``allow-recursion-on`` is not set, then ``allow-query-cache-on`` is
   used if set; otherwise, the default is to allow recursive queries on
   all addresses. Any client permitted to send recursive queries can
   send them to any address on which ``named`` is listening. Note: both
   ``allow-recursion`` and ``allow-recursion-on`` must be satisfied
   before recursion is allowed; a client that is blocked by one cannot
   be allowed by the other.

``allow-update``
   When set in the ``zone`` statement for a primary zone, this specifies which
   hosts are allowed to submit Dynamic DNS updates to that zone. The
   default is to deny updates from all hosts.

   Note that allowing updates based on the requestor's IP address is
   insecure; see :ref:`dynamic_update_security` for details.

   In general, this option should only be set at the ``zone`` level.
   While a default value can be set at the ``options`` or ``view`` level
   and inherited by zones, this could lead to some zones unintentionally
   allowing updates.

   Updates are written to the zone's filename that is set in ``file``.

``allow-update-forwarding``
   When set in the ``zone`` statement for a secondary zone, this specifies which
   hosts are allowed to submit Dynamic DNS updates and have them be
   forwarded to the primary. The default is ``{ none; }``, which means
   that no update forwarding is performed.

   To enable update forwarding, specify
   ``allow-update-forwarding { any; };`` in the ``zone`` statement.
   Specifying values other than ``{ none; }`` or ``{ any; }`` is usually
   counterproductive; the responsibility for update access control
   should rest with the primary server, not the secondary.

   Note that enabling the update forwarding feature on a secondary server
   may expose primary servers to attacks if they rely on insecure
   IP-address-based access control; see :ref:`dynamic_update_security` for more details.

   In general this option should only be set at the ``zone`` level.
   While a default value can be set at the ``options`` or ``view`` level
   and inherited by zones, this can lead to some zones unintentionally
   forwarding updates.

.. _allow-transfer-access:

``allow-transfer``
   This specifies which hosts are allowed to receive zone transfers from the
   server. ``allow-transfer`` may also be specified in the ``zone``
   statement, in which case it overrides the ``allow-transfer``
   statement set in ``options`` or ``view``. If not specified, the
   default is to allow transfers to all hosts.

``blackhole``
   This specifies a list of addresses which the server does not accept queries
   from or use to resolve a query. Queries from these addresses are not
   responded to. The default is ``none``.

``keep-response-order``
   This specifies a list of addresses to which the server sends responses
   to TCP queries, in the same order in which they were received. This
   disables the processing of TCP queries in parallel. The default is
   ``none``.

``no-case-compress``
   This specifies a list of addresses which require responses to use
   case-insensitive compression. This ACL can be used when ``named``
   needs to work with clients that do not comply with the requirement in
   :rfc:`1034` to use case-insensitive name comparisons when checking for
   matching domain names.

   If left undefined, the ACL defaults to ``none``: case-insensitive
   compression is used for all clients. If the ACL is defined and
   matches a client, case is ignored when compressing domain
   names in DNS responses sent to that client.

   This can result in slightly smaller responses; if a response contains
   the names "example.com" and "example.COM", case-insensitive
   compression treats the second one as a duplicate. It also
   ensures that the case of the query name exactly matches the case of
   the owner names of returned records, rather than matches the case of
   the records entered in the zone file. This allows responses to
   exactly match the query, which is required by some clients due to
   incorrect use of case-sensitive comparisons.

   Case-insensitive compression is *always* used in AXFR and IXFR
   responses, regardless of whether the client matches this ACL.

   There are circumstances in which ``named`` does not preserve the case
   of owner names of records: if a zone file defines records of
   different types with the same name, but the capitalization of the
   name is different (e.g., "www.example.com/A" and
   "WWW.EXAMPLE.COM/AAAA"), then all responses for that name use
   the *first* version of the name that was used in the zone file. This
   limitation may be addressed in a future release. However, domain
   names specified in the rdata of resource records (i.e., records of
   type NS, MX, CNAME, etc.) always have their case preserved unless
   the client matches this ACL.

``resolver-query-timeout``
   This is the amount of time in milliseconds that the resolver spends
   attempting to resolve a recursive query before failing. The default
   and minimum is ``10000`` and the maximum is ``30000``. Setting it to
   ``0`` results in the default being used.

   This value was originally specified in seconds. Values less than or
   equal to 300 are treated as seconds and converted to
   milliseconds before applying the above limits.

Interfaces
^^^^^^^^^^

The interfaces and ports that the server answers queries from may be
specified using the ``listen-on`` option. ``listen-on`` takes an
optional port and an ``address_match_list`` of IPv4 addresses. (IPv6
addresses are ignored, with a logged warning.) The server listens on
all interfaces allowed by the address match list. If a port is not
specified, port 53 is used.

Multiple ``listen-on`` statements are allowed. For example:

::

   listen-on { 5.6.7.8; };
   listen-on port 1234 { !1.2.3.4; 1.2/16; };

enables the name server on port 53 for the IP address 5.6.7.8, and
on port 1234 of an address on the machine in net 1.2 that is not
1.2.3.4.

If no ``listen-on`` is specified, the server listens on port 53 on
all IPv4 interfaces.

The ``listen-on-v6`` option is used to specify the interfaces and the
ports on which the server listens for incoming queries sent using
IPv6. If not specified, the server listens on port 53 on all IPv6
interfaces.

Multiple ``listen-on-v6`` options can be used. For example:

::

   listen-on-v6 { any; };
   listen-on-v6 port 1234 { !2001:db8::/32; any; };

enables the name server on port 53 for any IPv6 addresses (with a
single wildcard socket), and on port 1234 of IPv6 addresses that are not
in the prefix 2001:db8::/32 (with separate sockets for each matched
address).

To instruct the server not to listen on any IPv6 address, use:

::

   listen-on-v6 { none; };

.. _query_address:

Query Address
^^^^^^^^^^^^^

If the server does not know the answer to a question, it queries other
name servers. ``query-source`` specifies the address and port used for
such queries. For queries sent over IPv6, there is a separate
``query-source-v6`` option. If ``address`` is ``*`` (asterisk) or is
omitted, a wildcard IP address (``INADDR_ANY``) is used.

If ``port`` is ``*`` or is omitted, a random port number from a
pre-configured range is picked up and used for each query. The
port range(s) is specified in the ``use-v4-udp-ports`` (for IPv4)
and ``use-v6-udp-ports`` (for IPv6) options, excluding the ranges
specified in the ``avoid-v4-udp-ports`` and ``avoid-v6-udp-ports``
options, respectively.

The defaults of the ``query-source`` and ``query-source-v6`` options
are:

::

   query-source address * port *;
   query-source-v6 address * port *;

If ``use-v4-udp-ports`` or ``use-v6-udp-ports`` is unspecified,
``named`` checks whether the operating system provides a programming
interface to retrieve the system's default range for ephemeral ports. If
such an interface is available, ``named`` uses the corresponding
system default range; otherwise, it uses its own defaults:

::

   use-v4-udp-ports { range 1024 65535; };
   use-v6-udp-ports { range 1024 65535; };

.. note:: Make sure the ranges are sufficiently large for security. A
   desirable size depends on several parameters, but we generally recommend
   it contain at least 16384 ports (14 bits of entropy). Note also that the
   system's default range when used may be too small for this purpose, and
   that the range may even be changed while ``named`` is running; the new
   range is automatically applied when ``named`` is reloaded. Explicit
   configuration of ``use-v4-udp-ports`` and ``use-v6-udp-ports`` is encouraged,
   so that the ranges are sufficiently large and are reasonably
   independent from the ranges used by other applications.

.. note:: The operational configuration where ``named`` runs may prohibit
   the use of some ports. For example, Unix systems do not allow
   ``named``, if run without root privilege, to use ports less than 1024.
   If such ports are included in the specified (or detected) set of query
   ports, the corresponding query attempts will fail, resulting in
   resolution failures or delay. It is therefore important to configure the
   set of ports that can be safely used in the expected operational
   environment.

The defaults of the ``avoid-v4-udp-ports`` and ``avoid-v6-udp-ports``
options are:

::

   avoid-v4-udp-ports {};
   avoid-v6-udp-ports {};

.. note:: BIND 9.5.0 introduced the ``use-queryport-pool`` option to support
   a pool of such random ports, but this option is now obsolete because
   reusing the same ports in the pool may not be sufficiently secure. For
   the same reason, it is generally strongly discouraged to specify a
   particular port for the ``query-source`` or ``query-source-v6`` options;
   it implicitly disables the use of randomized port numbers.

``use-queryport-pool``
   This option is obsolete.

``queryport-pool-ports``
   This option is obsolete.

``queryport-pool-updateinterval``
   This option is obsolete.

.. note:: The address specified in the ``query-source`` option is used for both
   UDP and TCP queries, but the port applies only to UDP queries. TCP
   queries always use a random unprivileged port.

.. warning:: Specifying a single port is discouraged, as it removes a layer of
   protection against spoofing errors.

.. warning:: The configured ``port`` must not be same as the listening port.

.. note:: See also ``transfer-source``, ``notify-source`` and ``parental-source``.

.. _zone_transfers:

Zone Transfers
^^^^^^^^^^^^^^

BIND has mechanisms in place to facilitate zone transfers and set limits
on the amount of load that transfers place on the system. The following
options apply to zone transfers.

``also-notify``
   This option defines a global list of IP addresses of name servers that are also
   sent NOTIFY messages whenever a fresh copy of the zone is loaded, in
   addition to the servers listed in the zone's NS records. This helps
   to ensure that copies of the zones quickly converge on stealth
   servers. Optionally, a port may be specified with each
   ``also-notify`` address to send the notify messages to a port other
   than the default of 53. An optional TSIG key can also be specified
   with each address to cause the notify messages to be signed; this can
   be useful when sending notifies to multiple views. In place of
   explicit addresses, one or more named ``primaries`` lists can be used.

   If an ``also-notify`` list is given in a ``zone`` statement, it
   overrides the ``options also-notify`` statement. When a
   ``zone notify`` statement is set to ``no``, the IP addresses in the
   global ``also-notify`` list are not sent NOTIFY messages for that
   zone. The default is the empty list (no global notification list).

``max-transfer-time-in``
   Inbound zone transfers running longer than this many minutes are
   terminated. The default is 120 minutes (2 hours). The maximum value
   is 28 days (40320 minutes).

``max-transfer-idle-in``
   Inbound zone transfers making no progress in this many minutes are
   terminated. The default is 60 minutes (1 hour). The maximum value
   is 28 days (40320 minutes).

``max-transfer-time-out``
   Outbound zone transfers running longer than this many minutes are
   terminated. The default is 120 minutes (2 hours). The maximum value
   is 28 days (40320 minutes).

``max-transfer-idle-out``
   Outbound zone transfers making no progress in this many minutes are
   terminated. The default is 60 minutes (1 hour). The maximum value
   is 28 days (40320 minutes).

``notify-rate``
   This specifies the rate at which NOTIFY requests are sent during normal zone
   maintenance operations. (NOTIFY requests due to initial zone loading
   are subject to a separate rate limit; see below.) The default is 20
   per second. The lowest possible rate is one per second; when set to
   zero, it is silently raised to one.

``startup-notify-rate``
   This is the rate at which NOTIFY requests are sent when the name server
   is first starting up, or when zones have been newly added to the
   name server. The default is 20 per second. The lowest possible rate is
   one per second; when set to zero, it is silently raised to one.

``serial-query-rate``
   Secondary servers periodically query primary servers to find out if
   zone serial numbers have changed. Each such query uses a minute
   amount of the secondary server's network bandwidth. To limit the amount
   of bandwidth used, BIND 9 limits the rate at which queries are sent.
   The value of the ``serial-query-rate`` option, an integer, is the
   maximum number of queries sent per second. The default is 20 per
   second. The lowest possible rate is one per second; when set to zero,
   it is silently raised to one.

``transfer-format``
   Zone transfers can be sent using two different formats,
   ``one-answer`` and ``many-answers``. The ``transfer-format`` option
   is used on the primary server to determine which format it sends.
   ``one-answer`` uses one DNS message per resource record transferred.
   ``many-answers`` packs as many resource records as possible into one
   message. ``many-answers`` is more efficient; the default is ``many-answers``.
   ``transfer-format`` may be overridden on a per-server basis by using
   the ``server`` statement.

``transfer-message-size``
   This is an upper bound on the uncompressed size of DNS messages used
   in zone transfers over TCP. If a message grows larger than this size,
   additional messages are used to complete the zone transfer.
   (Note, however, that this is a hint, not a hard limit; if a message
   contains a single resource record whose RDATA does not fit within the
   size limit, a larger message will be permitted so the record can be
   transferred.)

   Valid values are between 512 and 65535 octets; any values outside
   that range are adjusted to the nearest value within it. The
   default is ``20480``, which was selected to improve message
   compression; most DNS messages of this size will compress to less
   than 16536 bytes. Larger messages cannot be compressed as
   effectively, because 16536 is the largest permissible compression
   offset pointer in a DNS message.

   This option is mainly intended for server testing; there is rarely
   any benefit in setting a value other than the default.

``transfers-in``
   This is the maximum number of inbound zone transfers that can run
   concurrently. The default value is ``10``. Increasing
   ``transfers-in`` may speed up the convergence of secondary zones, but it
   also may increase the load on the local system.

``transfers-out``
   This is the maximum number of outbound zone transfers that can run
   concurrently. Zone transfer requests in excess of the limit are
   refused. The default value is ``10``.

``transfers-per-ns``
   This is the maximum number of inbound zone transfers that can concurrently
   transfer from a given remote name server. The default value is
   ``2``. Increasing ``transfers-per-ns`` may speed up the convergence
   of secondary zones, but it also may increase the load on the remote name
   server. ``transfers-per-ns`` may be overridden on a per-server basis
   by using the ``transfers`` phrase of the ``server`` statement.

``transfer-source``
   ``transfer-source`` determines which local address is bound to
   IPv4 TCP connections used to fetch zones transferred inbound by the
   server. It also determines the source IPv4 address, and optionally
   the UDP port, used for the refresh queries and forwarded dynamic
   updates. If not set, it defaults to a system-controlled value which
   is usually the address of the interface "closest to" the remote
   end. This address must appear in the remote end's ``allow-transfer``
   option for the zone being transferred, if one is specified. This
   statement sets the ``transfer-source`` for all zones, but can be
   overridden on a per-view or per-zone basis by including a
   ``transfer-source`` statement within the ``view`` or ``zone`` block
   in the configuration file.

   .. warning:: Specifying a single port is discouraged, as it removes a layer of
      protection against spoofing errors.

   .. warning:: The configured ``port`` must not be same as the listening port.

``transfer-source-v6``
   This option is the same as ``transfer-source``, except zone transfers are performed
   using IPv6.

``alt-transfer-source``
   This indicates an alternate transfer source if the one listed in ``transfer-source``
   fails and ``use-alt-transfer-source`` is set.

   .. note:: To avoid using the alternate transfer source,
      set ``use-alt-transfer-source`` appropriately and
      do not depend upon getting an answer back to the first refresh
      query.

``alt-transfer-source-v6``
   This indicates an alternate transfer source if the one listed in
   ``transfer-source-v6`` fails and ``use-alt-transfer-source`` is set.

``use-alt-transfer-source``
   This indicates whether the alternate transfer sources should be used. If views are specified,
   this defaults to ``no``; otherwise, it defaults to ``yes``.

``notify-source``
   ``notify-source`` determines which local source address, and
   optionally UDP port, is used to send NOTIFY messages. This
   address must appear in the secondary server's ``primaries`` zone clause or
   in an ``allow-notify`` clause. This statement sets the
   ``notify-source`` for all zones, but can be overridden on a per-zone
   or per-view basis by including a ``notify-source`` statement within
   the ``zone`` or ``view`` block in the configuration file.

   .. warning:: Specifying a single port is discouraged, as it removes a layer of
      protection against spoofing errors.

   .. warning:: The configured ``port`` must not be same as the listening port.

``notify-source-v6``
   This option acts like ``notify-source``, but applies to notify messages sent to IPv6
   addresses.

.. _port_lists:

UDP Port Lists
^^^^^^^^^^^^^^

``use-v4-udp-ports``, ``avoid-v4-udp-ports``, ``use-v6-udp-ports``, and
``avoid-v6-udp-ports`` specify a list of IPv4 and IPv6 UDP ports that
are or are not used as source ports for UDP messages. See
:ref:`query_address` about how the available ports are
determined. For example, with the following configuration:

::

   use-v6-udp-ports { range 32768 65535; };
   avoid-v6-udp-ports { 40000; range 50000 60000; };

UDP ports of IPv6 messages sent from ``named`` are in one of the
following ranges: 32768 to 39999, 40001 to 49999, and 60001 to 65535.

``avoid-v4-udp-ports`` and ``avoid-v6-udp-ports`` can be used to prevent
``named`` from choosing as its random source port a port that is blocked
by a firewall or a port that is used by other applications; if a
query went out with a source port blocked by a firewall, the answer
would not pass through the firewall and the name server would have to query
again. Note: the desired range can also be represented only with
``use-v4-udp-ports`` and ``use-v6-udp-ports``, and the ``avoid-``
options are redundant in that sense; they are provided for backward
compatibility and to possibly simplify the port specification.

.. _resource_limits:

Operating System Resource Limits
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The server's usage of many system resources can be limited. Scaled
values are allowed when specifying resource limits. For example, ``1G``
can be used instead of ``1073741824`` to specify a limit of one
gigabyte. ``unlimited`` requests unlimited use, or the maximum available
amount. ``default`` uses the limit that was in force when the server was
started. See the description of ``size_spec`` in :ref:`configuration_file_elements`.

The following options set operating system resource limits for the name
server process. Some operating systems do not support some or any of the
limits; on such systems, a warning is issued if an unsupported
limit is used.

``coresize``
   This sets the maximum size of a core dump. The default is ``default``.

``datasize``
   This sets the maximum amount of data memory the server may use. The default is
   ``default``. This is a hard limit on server memory usage; if the
   server attempts to allocate memory in excess of this limit, the
   allocation will fail, which may in turn leave the server unable to
   perform DNS service. Therefore, this option is rarely useful as a way
   to limit the amount of memory used by the server, but it can be
   used to raise an operating system data size limit that is too small
   by default. To limit the amount of memory used by the
   server, use the ``max-cache-size`` and ``recursive-clients`` options
   instead.

``files``
   This sets the maximum number of files the server may have open concurrently.
   The default is ``unlimited``.

``stacksize``
   This sets the maximum amount of stack memory the server may use. The default is
   ``default``.

.. _server_resource_limits:

Server Resource Limits
^^^^^^^^^^^^^^^^^^^^^^

The following options set limits on the server's resource consumption
that are enforced internally by the server rather than by the operating
system.

``max-journal-size``
   This sets a maximum size for each journal file (see :ref:`journal`),
   expressed in bytes or, if followed by an
   optional unit suffix ('k', 'm', or 'g'), in kilobytes, megabytes, or
   gigabytes. When the journal file approaches the specified size, some
   of the oldest transactions in the journal are automatically
   removed. The largest permitted value is 2 gigabytes. Very small
   values are rounded up to 4096 bytes. It is possible to specify ``unlimited``,
   which also means 2 gigabytes. If the limit is set to ``default`` or
   left unset, the journal is allowed to grow up to twice as large
   as the zone. (There is little benefit in storing larger journals.)

   This option may also be set on a per-zone basis.

``max-records``
   This sets the maximum number of records permitted in a zone. The default is
   zero, which means the maximum is unlimited.

``recursive-clients``
   This sets the maximum number (a "hard quota") of simultaneous recursive lookups
   the server performs on behalf of clients. The default is
   ``1000``. Because each recursing client uses a fair bit of memory (on
   the order of 20 kilobytes), the value of the ``recursive-clients``
   option may have to be decreased on hosts with limited memory.

   ``recursive-clients`` defines a "hard quota" limit for pending
   recursive clients; when more clients than this are pending, new
   incoming requests are not accepted, and for each incoming request
   a previous pending request is dropped.

   A "soft quota" is also set. When this lower quota is exceeded,
   incoming requests are accepted, but for each one, a pending request
   is dropped. If ``recursive-clients`` is greater than 1000, the
   soft quota is set to ``recursive-clients`` minus 100; otherwise it is
   set to 90% of ``recursive-clients``.

``tcp-clients``
   This is the maximum number of simultaneous client TCP connections that the
   server accepts. The default is ``150``.

.. _clients-per-query:

``clients-per-query``; ``max-clients-per-query``
   These set the initial value (minimum) and maximum number of recursive
   simultaneous clients for any given query (<qname,qtype,qclass>) that
   the server accepts before dropping additional clients. ``named``
   attempts to self-tune this value and changes are logged. The
   default values are 10 and 100.

   This value should reflect how many queries come in for a given name
   in the time it takes to resolve that name. If the number of queries
   exceeds this value, ``named`` assumes that it is dealing with a
   non-responsive zone and drops additional queries. If it gets a
   response after dropping queries, it raises the estimate. The
   estimate is then lowered in 20 minutes if it has remained
   unchanged.

   If ``clients-per-query`` is set to zero, there is no limit on
   the number of clients per query and no queries are dropped.

   If ``max-clients-per-query`` is set to zero, there is no upper
   bound other than that imposed by ``recursive-clients``.

``fetches-per-zone``
   This sets the maximum number of simultaneous iterative queries to any one
   domain that the server permits before blocking new queries for
   data in or beneath that zone. This value should reflect how many
   fetches would normally be sent to any one zone in the time it would
   take to resolve them. It should be smaller than
   ``recursive-clients``.

   When many clients simultaneously query for the same name and type,
   the clients are all attached to the same fetch, up to the
   ``max-clients-per-query`` limit, and only one iterative query is
   sent. However, when clients are simultaneously querying for
   *different* names or types, multiple queries are sent and
   ``max-clients-per-query`` is not effective as a limit.

   Optionally, this value may be followed by the keyword ``drop`` or
   ``fail``, indicating whether queries which exceed the fetch quota for
   a zone are dropped with no response, or answered with SERVFAIL.
   The default is ``drop``.

   If ``fetches-per-zone`` is set to zero, there is no limit on the
   number of fetches per query and no queries are dropped. The
   default is zero.

   The current list of active fetches can be dumped by running
   ``rndc recursing``. The list includes the number of active fetches
   for each domain and the number of queries that have been passed
   (allowed) or dropped (spilled) as a result of the ``fetches-per-zone``
   limit. (Note: these counters are not cumulative over time;
   whenever the number of active fetches for a domain drops to zero,
   the counter for that domain is deleted, and the next time a fetch
   is sent to that domain, it is recreated with the counters set
   to zero.)

``fetches-per-server``
   This sets the maximum number of simultaneous iterative queries that the server
   allows to be sent to a single upstream name server before
   blocking additional queries. This value should reflect how many
   fetches would normally be sent to any one server in the time it would
   take to resolve them. It should be smaller than
   ``recursive-clients``.

   Optionally, this value may be followed by the keyword ``drop`` or
   ``fail``, indicating whether queries are dropped with no
   response or answered with SERVFAIL, when all of the servers
   authoritative for a zone are found to have exceeded the per-server
   quota. The default is ``fail``.

   If ``fetches-per-server`` is set to zero, there is no limit on
   the number of fetches per query and no queries are dropped. The
   default is zero.

   The ``fetches-per-server`` quota is dynamically adjusted in response
   to detected congestion. As queries are sent to a server and either are
   answered or time out, an exponentially weighted moving average
   is calculated of the ratio of timeouts to responses. If the current
   average timeout ratio rises above a "high" threshold, then
   ``fetches-per-server`` is reduced for that server. If the timeout
   ratio drops below a "low" threshold, then ``fetches-per-server`` is
   increased. The ``fetch-quota-params`` options can be used to adjust
   the parameters for this calculation.

``fetch-quota-params``
   This sets the parameters to use for dynamic resizing of the
   ``fetches-per-server`` quota in response to detected congestion.

   The first argument is an integer value indicating how frequently to
   recalculate the moving average of the ratio of timeouts to responses
   for each server. The default is 100, meaning that BIND recalculates the
   average ratio after every 100 queries have either been answered or
   timed out.

   The remaining three arguments represent the "low" threshold
   (defaulting to a timeout ratio of 0.1), the "high" threshold
   (defaulting to a timeout ratio of 0.3), and the discount rate for the
   moving average (defaulting to 0.7). A higher discount rate causes
   recent events to weigh more heavily when calculating the moving
   average; a lower discount rate causes past events to weigh more
   heavily, smoothing out short-term blips in the timeout ratio. These
   arguments are all fixed-point numbers with precision of 1/100; at
   most two places after the decimal point are significant.

``reserved-sockets``
   This sets the number of file descriptors reserved for TCP, stdio, etc. This
   needs to be big enough to cover the number of interfaces ``named``
   listens on plus ``tcp-clients``, as well as to provide room for
   outgoing TCP queries and incoming zone transfers. The default is
   ``512``. The minimum value is ``128`` and the maximum value is
   ``128`` fewer than maxsockets (-S). This option may be removed in the
   future.

   This option has little effect on Windows.

``max-cache-size``
   This sets the maximum amount of memory to use for an individual cache
   database and its associated metadata, in bytes or percentage of total
   physical memory. By default, each view has its own separate cache,
   which means the total amount of memory required for cache data is the
   sum of the cache database sizes for all views (unless the
   :ref:`attach-cache <attach-cache>` option is used).

   When the amount of data in a cache database reaches the configured
   limit, ``named`` starts purging non-expired records (following an
   LRU-based strategy).

   The default size limit for each individual cache is:

     - 90% of physical memory for views with ``recursion`` set to
       ``yes`` (the default), or

     - 2 MB for views with ``recursion`` set to ``no``.

   Any positive value smaller than 2 MB is ignored and reset to 2 MB.
   The keyword ``unlimited``, or the value ``0``, places no limit on the
   cache size; records are then purged from the cache only when they
   expire (according to their TTLs).

   .. note::

       For configurations which define multiple views with separate
       caches and recursion enabled, it is recommended to set
       ``max-cache-size`` appropriately for each view, as using the
       default value of that option (90% of physical memory for each
       individual cache) may lead to memory exhaustion over time.

   Upon startup and reconfiguration, caches with a limited size
   preallocate a small amount of memory (less than 1% of
   ``max-cache-size`` for a given view). This preallocation serves as an
   optimization to eliminate extra latency introduced by resizing
   internal cache structures.

   On systems where detection of the amount of physical memory is not
   supported, percentage-based values fall back to ``unlimited``. Note
   that the amount of physical memory available is only detected on
   startup, so ``named`` does not adjust the cache size limits if the
   amount of physical memory is changed at runtime.

``tcp-listen-queue``
   This sets the listen-queue depth. The default and minimum is 10. If the kernel
   supports the accept filter "dataready", this also controls how many
   TCP connections are queued in kernel space waiting for some
   data before being passed to accept. Non-zero values less than 10 are
   silently raised. A value of 0 may also be used; on most platforms
   this sets the listen-queue length to a system-defined default value.

``tcp-initial-timeout``
   This sets the amount of time (in units of 100 milliseconds) that the server waits on
   a new TCP connection for the first message from the client. The
   default is 300 (30 seconds), the minimum is 25 (2.5 seconds), and the
   maximum is 1200 (two minutes). Values above the maximum or below the
   minimum are adjusted with a logged warning. (Note: this value
   must be greater than the expected round-trip delay time; otherwise, no
   client will ever have enough time to submit a message.) This value
   can be updated at runtime by using ``rndc tcp-timeouts``.

``tcp-idle-timeout``
   This sets the amount of time (in units of 100 milliseconds) that the server waits on
   an idle TCP connection before closing it, when the client is not using
   the EDNS TCP keepalive option. The default is 300 (30 seconds), the
   maximum is 1200 (two minutes), and the minimum is 1 (one-tenth of a
   second). Values above the maximum or below the minimum are
   adjusted with a logged warning. See ``tcp-keepalive-timeout`` for
   clients using the EDNS TCP keepalive option. This value can be
   updated at runtime by using ``rndc tcp-timeouts``.

``tcp-keepalive-timeout``
   This sets the amount of time (in units of 100 milliseconds) that the server waits on
   an idle TCP connection before closing it, when the client is using the
   EDNS TCP keepalive option. The default is 300 (30 seconds), the
   maximum is 65535 (about 1.8 hours), and the minimum is 1 (one-tenth
   of a second). Values above the maximum or below the minimum are
   adjusted with a logged warning. This value may be greater than
   ``tcp-idle-timeout`` because clients using the EDNS TCP keepalive
   option are expected to use TCP connections for more than one message.
   This value can be updated at runtime by using ``rndc tcp-timeouts``.

``tcp-advertised-timeout``
   This sets the timeout value (in units of 100 milliseconds) that the server sends
   in responses containing the EDNS TCP keepalive option, which informs a
   client of the amount of time it may keep the session open. The
   default is 300 (30 seconds), the maximum is 65535 (about 1.8 hours),
   and the minimum is 0, which signals that the clients must close TCP
   connections immediately. Ordinarily this should be set to the same
   value as ``tcp-keepalive-timeout``. This value can be updated at
   runtime by using ``rndc tcp-timeouts``.

``update-quota``
   This is the maximum number of simultaneous DNS UPDATE messages that
   the server will accept for updating local authoritiative zones or
   forwarding to a primary server. The default is ``100``.

.. _intervals:

Periodic Task Intervals
^^^^^^^^^^^^^^^^^^^^^^^

``cleaning-interval``
   This option is obsolete.

``heartbeat-interval``
   The server performs zone maintenance tasks for all zones marked
   as ``dialup`` whenever this interval expires. The default is 60
   minutes. Reasonable values are up to 1 day (1440 minutes). The
   maximum value is 28 days (40320 minutes). If set to 0, no zone
   maintenance for these zones occurs.

``interface-interval``
   The server scans the network interface list every ``interface-interval``
   minutes. The default is 60 minutes; the maximum value is 28 days (40320
   minutes). If set to 0, interface scanning only occurs when the configuration
   file is loaded, or when ``automatic-interface-scan`` is enabled and supported
   by the operating system. After the scan, the server begins listening for
   queries on any newly discovered interfaces (provided they are allowed by the
   ``listen-on`` configuration), and stops listening on interfaces that have
   gone away. For convenience, TTL-style time-unit suffixes may be used to
   specify the value. It also accepts ISO 8601 duration formats.

.. _the_sortlist_statement:

The ``sortlist`` Statement
^^^^^^^^^^^^^^^^^^^^^^^^^^

The response to a DNS query may consist of multiple resource records
(RRs) forming a resource record set (RRset). The name server
normally returns the RRs within the RRset in an indeterminate order (but
see the ``rrset-order`` statement in :ref:`rrset_ordering`). The client resolver code should
rearrange the RRs as appropriate: that is, using any addresses on the
local net in preference to other addresses. However, not all resolvers
can do this or are correctly configured. When a client is using a local
server, the sorting can be performed in the server, based on the
client's address. This only requires configuring the name servers, not
all the clients.

The ``sortlist`` statement (see below) takes an ``address_match_list`` and
interprets it in a special way. Each top-level statement in the ``sortlist``
must itself be an explicit ``address_match_list`` with one or two elements. The
first element (which may be an IP address, an IP prefix, an ACL name, or a nested
``address_match_list``) of each top-level list is checked against the source
address of the query until a match is found. When the addresses in the first
element overlap, the first rule to match is selected.

Once the source address of the query has been matched, if the top-level
statement contains only one element, the actual primitive element that
matched the source address is used to select the address in the response
to move to the beginning of the response. If the statement is a list of
two elements, then the second element is interpreted as a topology
preference list. Each top-level element is assigned a distance, and the
address in the response with the minimum distance is moved to the
beginning of the response.

In the following example, any queries received from any of the addresses
of the host itself get responses preferring addresses on any of the
locally connected networks. Next most preferred are addresses on the
192.168.1/24 network, and after that either the 192.168.2/24 or
192.168.3/24 network, with no preference shown between these two
networks. Queries received from a host on the 192.168.1/24 network
prefer other addresses on that network to the 192.168.2/24 and
192.168.3/24 networks. Queries received from a host on the 192.168.4/24
or the 192.168.5/24 network only prefer other addresses on their
directly connected networks.

::

   sortlist {
       // IF the local host
       // THEN first fit on the following nets
       { localhost;
       { localnets;
           192.168.1/24;
           { 192.168.2/24; 192.168.3/24; }; }; };
       // IF on class C 192.168.1 THEN use .1, or .2 or .3
       { 192.168.1/24;
       { 192.168.1/24;
           { 192.168.2/24; 192.168.3/24; }; }; };
       // IF on class C 192.168.2 THEN use .2, or .1 or .3
       { 192.168.2/24;
       { 192.168.2/24;
           { 192.168.1/24; 192.168.3/24; }; }; };
       // IF on class C 192.168.3 THEN use .3, or .1 or .2
       { 192.168.3/24;
       { 192.168.3/24;
           { 192.168.1/24; 192.168.2/24; }; }; };
       // IF .4 or .5 THEN prefer that net
       { { 192.168.4/24; 192.168.5/24; };
       };
   };

The following example illlustrates reasonable behavior for the local host
and hosts on directly connected networks. Responses sent to queries from the
local host favor any of the directly connected networks. Responses
sent to queries from any other hosts on a directly connected network
prefer addresses on that same network. Responses to other queries
are not sorted.

::

   sortlist {
          { localhost; localnets; };
          { localnets; };
   };

.. _rrset_ordering:

RRset Ordering
^^^^^^^^^^^^^^

.. note::

    While alternating the order of records in a DNS response between
    subsequent queries is a known load distribution technique, certain
    caveats apply (mostly stemming from caching) which usually make it a
    suboptimal choice for load balancing purposes when used on its own.

The ``rrset-order`` statement permits configuration of the ordering of
the records in a multiple-record response. See also:
:ref:`the_sortlist_statement`.

Each rule in an ``rrset-order`` statement is defined as follows:

::

    [class <class_name>] [type <type_name>] [name "<domain_name>"] order <ordering>

The default qualifiers for each rule are:

  - If no ``class`` is specified, the default is ``ANY``.
  - If no ``type`` is specified, the default is ``ANY``.
  - If no ``name`` is specified, the default is ``*`` (asterisk).

``<domain_name>`` only matches the name itself, not any of its
subdomains.  To make a rule match all subdomains of a given name, a
wildcard name (``*.<domain_name>``) must be used.  Note that
``*.<domain_name>`` does *not* match ``<domain_name>`` itself; to
specify RRset ordering for a name and all of its subdomains, two
separate rules must be defined: one for ``<domain_name>`` and one for
``*.<domain_name>``.

The legal values for ``<ordering>`` are:

``fixed``
    Records are returned in the order they are defined in the zone file.

.. note::

    The ``fixed`` option is only available if BIND is configured with
    ``--enable-fixed-rrset`` at compile time.

``random``
    Records are returned in a random order.

``cyclic``
    Records are returned in a cyclic round-robin order, rotating by one
    record per query.

``none``
    Records are returned in the order they were retrieved from the
    database. This order is indeterminate, but remains consistent as
    long as the database is not modified.

The default RRset order used depends on whether any ``rrset-order``
statements are present in the configuration file used by ``named``:

  - If no ``rrset-order`` statement is present in the configuration
    file, the implicit default is to return all records in ``random``
    order.

  - If any ``rrset-order`` statements are present in the configuration
    file, but no ordering rule specified in these statements matches a
    given RRset, the default order for that RRset is ``none``.

Note that if multiple ``rrset-order`` statements are present in the
configuration file (at both the ``options`` and ``view`` levels), they
are *not* combined; instead, the more-specific one (``view``) replaces
the less-specific one (``options``).

If multiple rules within a single ``rrset-order`` statement match a
given RRset, the first matching rule is applied.

Example:

::

    rrset-order {
        type A name "foo.isc.org" order random;
        type AAAA name "foo.isc.org" order cyclic;
        name "bar.isc.org" order fixed;
        name "*.bar.isc.org" order random;
        name "*.baz.isc.org" order cyclic;
    };

With the above configuration, the following RRset ordering is used:

===================    ========    ===========
QNAME                  QTYPE       RRset Order
===================    ========    ===========
``foo.isc.org``        ``A``       ``random``
``foo.isc.org``        ``AAAA``    ``cyclic``
``foo.isc.org``        ``TXT``     ``none``
``sub.foo.isc.org``    all         ``none``
``bar.isc.org``        all         ``fixed``
``sub.bar.isc.org``    all         ``random``
``baz.isc.org``        all         ``none``
``sub.baz.isc.org``    all         ``cyclic``
===================    ========    ===========

.. _tuning:

Tuning
^^^^^^

``lame-ttl``
   This is always set to 0. More information is available in the
   `security advisory for CVE-2021-25219
   <https://kb.isc.org/docs/cve-2021-25219>`_.

``servfail-ttl``
   This sets the number of seconds to cache a SERVFAIL response due to DNSSEC
   validation failure or other general server failure. If set to ``0``,
   SERVFAIL caching is disabled. The SERVFAIL cache is not consulted if
   a query has the CD (Checking Disabled) bit set; this allows a query
   that failed due to DNSSEC validation to be retried without waiting
   for the SERVFAIL TTL to expire.

   The maximum value is ``30`` seconds; any higher value is
   silently reduced. The default is ``1`` second.

``min-ncache-ttl``
   To reduce network traffic and increase performance, the server stores
   negative answers. ``min-ncache-ttl`` is used to set a minimum
   retention time for these answers in the server, in seconds. For
   convenience, TTL-style time-unit suffixes may be used to specify the
   value. It also accepts ISO 8601 duration formats.

   The default ``min-ncache-ttl`` is ``0`` seconds. ``min-ncache-ttl`` cannot
   exceed 90 seconds and is truncated to 90 seconds if set to a greater
   value.

``min-cache-ttl``
   This sets the minimum time for which the server caches ordinary (positive)
   answers, in seconds. For convenience, TTL-style time-unit suffixes may be used
   to specify the value. It also accepts ISO 8601 duration formats.

   The default ``min-cache-ttl`` is ``0`` seconds. ``min-cache-ttl`` cannot
   exceed 90 seconds and is truncated to 90 seconds if set to a greater
   value.

``max-ncache-ttl``
   To reduce network traffic and increase performance, the server stores
   negative answers. ``max-ncache-ttl`` is used to set a maximum retention time
   for these answers in the server, in seconds. For convenience, TTL-style
   time-unit suffixes may be used to specify the value.  It also accepts ISO 8601
   duration formats.

   The default ``max-ncache-ttl`` is 10800 seconds (3 hours). ``max-ncache-ttl``
   cannot exceed 7 days and is silently truncated to 7 days if set to a
   greater value.

``max-cache-ttl``
   This sets the maximum time for which the server caches ordinary (positive)
   answers, in seconds. For convenience, TTL-style time-unit suffixes may be used
   to specify the value. It also accepts ISO 8601 duration formats.

   The default ``max-cache-ttl`` is 604800 (one week). A value of zero may cause
   all queries to return SERVFAIL, because of lost caches of intermediate RRsets
   (such as NS and glue AAAA/A records) in the resolution process.

``max-stale-ttl``
   If retaining stale RRsets in cache is enabled, and returning of stale cached
   answers is also enabled, ``max-stale-ttl`` sets the maximum time for which
   the server retains records past their normal expiry to return them as stale
   records, when the servers for those records are not reachable. The default
   is 1 day. The minimum allowed is 1 second; a value of 0 is updated silently
   to 1 second.

   For stale answers to be returned, the retaining of them in cache must be
   enabled via the configuration option ``stale-cache-enable``, and returning
   cached answers must be enabled, either in the configuration file using the
   ``stale-answer-enable`` option or by calling ``rndc serve-stale on``.

   When ``stale-cache-enable`` is set to ``no``, setting the ``max-stale-ttl``
   has no effect, the value of ``max-cache-ttl`` will be ``0`` in such case.

``resolver-nonbackoff-tries``
   This specifies how many retries occur before exponential backoff kicks in. The
   default is ``3``.

``resolver-retry-interval``
   This sets the base retry interval in milliseconds. The default is ``800``.

``sig-validity-interval``
   this specifies the upper bound of the number of days that RRSIGs
   generated by ``named`` are valid; the default is ``30`` days,
   with a maximum of 3660 days (10 years). The optional second value
   specifies the minimum bound on those RRSIGs and also determines
   how long before expiry ``named`` starts regenerating those RRSIGs.
   The default value for the lower bound is 1/4 of the upper bound;
   it is expressed in days if the upper bound is greater than 7,
   and hours if it is less than or equal to 7 days.

   When new RRSIGs are generated, the length of time is randomly
   chosen between these two limits, to spread out the re-signing
   load. When RRSIGs are re-generated, the upper bound is used, with
   a small amount of jitter added. New RRSIGs are generated by a
   number of processes, including the processing of UPDATE requests
   (ref:`dynamic_update`), the addition and removal of records via
   in-line signing, and the initial signing of a zone.

   The signature inception time is unconditionally set to one hour
   before the current time, to allow for a limited amount of clock skew.

   The ``sig-validity-interval`` can be overridden for DNSKEY records by
   setting ``dnskey-sig-validity``.

   The ``sig-validity-interval`` should be at least several multiples
   of the SOA expire interval, to allow for reasonable interaction
   between the various timer and expiry dates.

``dnskey-sig-validity``
   This specifies the number of days into the future when DNSSEC signatures
   that are automatically generated for DNSKEY RRsets as a result of
   dynamic updates (:ref:`dynamic_update`) will expire.
   If set to a non-zero value, this overrides the value set by
   ``sig-validity-interval``. The default is zero, meaning
   ``sig-validity-interval`` is used. The maximum value is 3660 days (10
   years), and higher values are rejected.

``sig-signing-nodes``
   This specifies the maximum number of nodes to be examined in each quantum,
   when signing a zone with a new DNSKEY. The default is ``100``.

``sig-signing-signatures``
   This specifies a threshold number of signatures that terminates
   processing a quantum, when signing a zone with a new DNSKEY. The
   default is ``10``.

``sig-signing-type``
   This specifies a private RDATA type to be used when generating signing-state
   records. The default is ``65534``.

   This parameter may be removed in a future version,
   once there is a standard type.

   Signing-state records are used internally by ``named`` to track
   the current state of a zone-signing process, i.e., whether it is
   still active or has been completed. The records can be inspected
   using the command ``rndc signing -list zone``. Once ``named`` has
   finished signing a zone with a particular key, the signing-state
   record associated with that key can be removed from the zone by
   running ``rndc signing -clear keyid/algorithm zone``. To clear all of
   the completed signing-state records for a zone, use
   ``rndc signing -clear all zone``.

``min-refresh-time``; ``max-refresh-time``; ``min-retry-time``; ``max-retry-time``
   These options control the server's behavior on refreshing a zone
   (querying for SOA changes) or retrying failed transfers. Usually the
   SOA values for the zone are used, up to a hard-coded maximum expiry
   of 24 weeks. However, these values are set by the primary, giving
   secondary server administrators little control over their contents.

   These options allow the administrator to set a minimum and maximum
   refresh and retry time in seconds per-zone, per-view, or globally.
   These options are valid for secondary and stub zones, and clamp the SOA
   refresh and retry times to the specified values.

   The following defaults apply: ``min-refresh-time`` 300 seconds,
   ``max-refresh-time`` 2419200 seconds (4 weeks), ``min-retry-time``
   500 seconds, and ``max-retry-time`` 1209600 seconds (2 weeks).

``edns-udp-size``
   This sets the maximum advertised EDNS UDP buffer size, in bytes, to control
   the size of packets received from authoritative servers in response
   to recursive queries. Valid values are 512 to 4096; values outside
   this range are silently adjusted to the nearest value within it.
   The default value is 1232.

   The usual reason for setting ``edns-udp-size`` to a non-default value
   is to get UDP answers to pass through broken firewalls that block
   fragmented packets and/or block UDP DNS packets that are greater than
   512 bytes.

   When ``named`` first queries a remote server, it advertises a UDP
   buffer size of 512, as this has the greatest chance of success on the
   first try.

   If the initial query is successful with EDNS advertising a buffer size of
   512, then ``named`` will advertise progressively larger buffer sizes on
   successive queries, until responses begin timing out or ``edns-udp-size`` is
   reached.

   The default buffer sizes used by ``named`` are 512, 1232, 1432, and
   4096, but never exceeding ``edns-udp-size``. (The values 1232 and
   1432 are chosen to allow for an IPv4-/IPv6-encapsulated UDP message
   to be sent without fragmentation at the minimum MTU sizes for
   Ethernet and IPv6 networks.)

   The ``named`` now sets the DON'T FRAGMENT flag on outgoing UDP packets.
   According to the measurements done by multiple parties this should not be
   causing any operational problems as most of the Internet "core" is able to
   cope with IP message sizes between 1400-1500 bytes, the 1232 size was picked
   as a conservative minimal number that could be changed by the DNS operator to
   a estimated path MTU minus the estimated header space. In practice, the
   smallest MTU witnessed in the operational DNS community is 1500 octets, the
   Ethernet maximum payload size, so a a useful default for maximum DNS/UDP
   payload size on **reliable** networks would be 1432.

   Any server-specific ``edns-udp-size`` setting has precedence over all
   the above rules.

``max-udp-size``
   This sets the maximum EDNS UDP message size that ``named`` sends, in bytes.
   Valid values are 512 to 4096; values outside this range are
   silently adjusted to the nearest value within it. The default value
   is 1232.

   This value applies to responses sent by a server; to set the
   advertised buffer size in queries, see ``edns-udp-size``.

   The usual reason for setting ``max-udp-size`` to a non-default value
   is to allow UDP answers to pass through broken firewalls that block
   fragmented packets and/or block UDP packets that are greater than 512
   bytes. This is independent of the advertised receive buffer
   (``edns-udp-size``).

   Setting this to a low value encourages additional TCP traffic to
   the name server.

``masterfile-format``
   This specifies the file format of zone files (see :ref:`zonefile_format`
   for details).  The default value is ``text``, which is the standard
   textual representation, except for secondary zones, in which the default
   value is ``raw``. Files in formats other than ``text`` are typically
   expected to be generated by the ``named-compilezone`` tool, or dumped by
   ``named``.

   Note that when a zone file in a format other than ``text`` is loaded,
   ``named`` may omit some of the checks which are performed for a file in
   ``text`` format. For example, ``check-names`` only applies when loading
   zones in ``text`` format, and ``max-zone-ttl`` only applies to ``text``
   and ``raw``.  Zone files in binary formats should be generated with the
   same check level as that specified in the ``named`` configuration file.

   ``map`` format files are loaded directly into memory via memory mapping,
   with only minimal validity checking. Because they are not guaranteed to
   be compatible from one version of BIND 9 to another, and are not
   compatible from one system architecture to another, they should be used
   with caution. See :ref:`zonefile_format` for further discussion.

   When configured in ``options``, this statement sets the
   ``masterfile-format`` for all zones, but it can be overridden on a
   per-zone or per-view basis by including a ``masterfile-format``
   statement within the ``zone`` or ``view`` block in the configuration
   file.

``masterfile-style``
   This specifies the formatting of zone files during dump, when the
   ``masterfile-format`` is ``text``. This option is ignored with any
   other ``masterfile-format``.

   When set to ``relative``, records are printed in a multi-line format,
   with owner names expressed relative to a shared origin. When set to
   ``full``, records are printed in a single-line format with absolute
   owner names. The ``full`` format is most suitable when a zone file
   needs to be processed automatically by a script. The ``relative``
   format is more human-readable, and is thus suitable when a zone is to
   be edited by hand. The default is ``relative``.

``max-recursion-depth``
   This sets the maximum number of levels of recursion that are permitted at
   any one time while servicing a recursive query. Resolving a name may
   require looking up a name server address, which in turn requires
   resolving another name, etc.; if the number of recursions exceeds
   this value, the recursive query is terminated and returns SERVFAIL.
   The default is 7.

``max-recursion-queries``
   This sets the maximum number of iterative queries that may be sent while
   servicing a recursive query. If more queries are sent, the recursive
   query is terminated and returns SERVFAIL. The default is 100.

``notify-delay``
   This sets the delay, in seconds, between sending sets of NOTIFY messages
   for a zone. Whenever a NOTIFY message is sent for a zone, a timer will
   be set for this duration. If the zone is updated again before the timer
   expires, the NOTIFY for that update will be postponed. The default is 5
   seconds.

   The overall rate at which NOTIFY messages are sent for all zones is
   controlled by ``notify-rate``.

``max-rsa-exponent-size``
   This sets the maximum RSA exponent size, in bits, that is accepted when
   validating. Valid values are 35 to 4096 bits. The default, zero, is
   also accepted and is equivalent to 4096.

``prefetch``
   When a query is received for cached data which is to expire shortly,
   ``named`` can refresh the data from the authoritative server
   immediately, ensuring that the cache always has an answer available.

   ``prefetch`` specifies the "trigger" TTL value at which prefetch
   of the current query takes place; when a cache record with a
   lower or equal TTL value is encountered during query processing, it is
   refreshed. Valid trigger TTL values are 1 to 10 seconds. Values
   larger than 10 seconds are silently reduced to 10. Setting a
   trigger TTL to zero causes prefetch to be disabled. The default
   trigger TTL is ``2``.

   An optional second argument specifies the "eligibility" TTL: the
   smallest *original* TTL value that is accepted for a record to
   be eligible for prefetching. The eligibility TTL must be at least six
   seconds longer than the trigger TTL; if not, ``named``
   silently adjusts it upward. The default eligibility TTL is ``9``.

``v6-bias``
   When determining the next name server to try, this indicates by how many
   milliseconds to prefer IPv6 name servers. The default is ``50``
   milliseconds.

.. _builtin:

Built-in Server Information Zones
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The server provides some helpful diagnostic information through a number
of built-in zones under the pseudo-top-level-domain ``bind`` in the
``CHAOS`` class. These zones are part of a built-in view
(see :ref:`view_statement_grammar`) of class ``CHAOS``, which is
separate from the default view of class ``IN``. Most global
configuration options (``allow-query``, etc.) apply to this view,
but some are locally overridden: ``notify``, ``recursion``, and
``allow-new-zones`` are always set to ``no``, and ``rate-limit`` is set
to allow three responses per second.

To disable these zones, use the options below or hide the
built-in ``CHAOS`` view by defining an explicit view of class ``CHAOS``
that matches all clients.

``version``
   This is the version the server should report via a query of the name
   ``version.bind`` with type ``TXT`` and class ``CHAOS``. The default is
   the real version number of this server. Specifying ``version none``
   disables processing of the queries.

   Setting ``version`` to any value (including ``none``) also disables
   queries for ``authors.bind TXT CH``.

``hostname``
   This is the hostname the server should report via a query of the name
   ``hostname.bind`` with type ``TXT`` and class ``CHAOS``. This defaults
   to the hostname of the machine hosting the name server, as found by
   the ``gethostname()`` function. The primary purpose of such queries is to
   identify which of a group of anycast servers is actually answering
   the queries. Specifying ``hostname none;`` disables processing of
   the queries.

``server-id``
   This is the ID the server should report when receiving a Name Server
   Identifier (NSID) query, or a query of the name ``ID.SERVER`` with
   type ``TXT`` and class ``CHAOS``. The primary purpose of such queries is
   to identify which of a group of anycast servers is actually answering
   the queries. Specifying ``server-id none;`` disables processing of
   the queries. Specifying ``server-id hostname;`` causes ``named``
   to use the hostname as found by the ``gethostname()`` function. The
   default ``server-id`` is ``none``.

.. _empty:

Built-in Empty Zones
^^^^^^^^^^^^^^^^^^^^

The ``named`` server has some built-in empty zones, for SOA and NS records
only. These are for zones that should normally be answered locally and for
which queries should not be sent to the Internet's root servers. The
official servers that cover these namespaces return NXDOMAIN responses
to these queries. In particular, these cover the reverse namespaces for
addresses from :rfc:`1918`, :rfc:`4193`, :rfc:`5737`, and :rfc:`6598`. They also
include the reverse namespace for the IPv6 local address (locally assigned),
IPv6 link local addresses, the IPv6 loopback address, and the IPv6
unknown address.

The server attempts to determine if a built-in zone already exists
or is active (covered by a forward-only forwarding declaration) and does
not create an empty zone if either is true.

The current list of empty zones is:

-  10.IN-ADDR.ARPA
-  16.172.IN-ADDR.ARPA
-  17.172.IN-ADDR.ARPA
-  18.172.IN-ADDR.ARPA
-  19.172.IN-ADDR.ARPA
-  20.172.IN-ADDR.ARPA
-  21.172.IN-ADDR.ARPA
-  22.172.IN-ADDR.ARPA
-  23.172.IN-ADDR.ARPA
-  24.172.IN-ADDR.ARPA
-  25.172.IN-ADDR.ARPA
-  26.172.IN-ADDR.ARPA
-  27.172.IN-ADDR.ARPA
-  28.172.IN-ADDR.ARPA
-  29.172.IN-ADDR.ARPA
-  30.172.IN-ADDR.ARPA
-  31.172.IN-ADDR.ARPA
-  168.192.IN-ADDR.ARPA
-  64.100.IN-ADDR.ARPA
-  65.100.IN-ADDR.ARPA
-  66.100.IN-ADDR.ARPA
-  67.100.IN-ADDR.ARPA
-  68.100.IN-ADDR.ARPA
-  69.100.IN-ADDR.ARPA
-  70.100.IN-ADDR.ARPA
-  71.100.IN-ADDR.ARPA
-  72.100.IN-ADDR.ARPA
-  73.100.IN-ADDR.ARPA
-  74.100.IN-ADDR.ARPA
-  75.100.IN-ADDR.ARPA
-  76.100.IN-ADDR.ARPA
-  77.100.IN-ADDR.ARPA
-  78.100.IN-ADDR.ARPA
-  79.100.IN-ADDR.ARPA
-  80.100.IN-ADDR.ARPA
-  81.100.IN-ADDR.ARPA
-  82.100.IN-ADDR.ARPA
-  83.100.IN-ADDR.ARPA
-  84.100.IN-ADDR.ARPA
-  85.100.IN-ADDR.ARPA
-  86.100.IN-ADDR.ARPA
-  87.100.IN-ADDR.ARPA
-  88.100.IN-ADDR.ARPA
-  89.100.IN-ADDR.ARPA
-  90.100.IN-ADDR.ARPA
-  91.100.IN-ADDR.ARPA
-  92.100.IN-ADDR.ARPA
-  93.100.IN-ADDR.ARPA
-  94.100.IN-ADDR.ARPA
-  95.100.IN-ADDR.ARPA
-  96.100.IN-ADDR.ARPA
-  97.100.IN-ADDR.ARPA
-  98.100.IN-ADDR.ARPA
-  99.100.IN-ADDR.ARPA
-  100.100.IN-ADDR.ARPA
-  101.100.IN-ADDR.ARPA
-  102.100.IN-ADDR.ARPA
-  103.100.IN-ADDR.ARPA
-  104.100.IN-ADDR.ARPA
-  105.100.IN-ADDR.ARPA
-  106.100.IN-ADDR.ARPA
-  107.100.IN-ADDR.ARPA
-  108.100.IN-ADDR.ARPA
-  109.100.IN-ADDR.ARPA
-  110.100.IN-ADDR.ARPA
-  111.100.IN-ADDR.ARPA
-  112.100.IN-ADDR.ARPA
-  113.100.IN-ADDR.ARPA
-  114.100.IN-ADDR.ARPA
-  115.100.IN-ADDR.ARPA
-  116.100.IN-ADDR.ARPA
-  117.100.IN-ADDR.ARPA
-  118.100.IN-ADDR.ARPA
-  119.100.IN-ADDR.ARPA
-  120.100.IN-ADDR.ARPA
-  121.100.IN-ADDR.ARPA
-  122.100.IN-ADDR.ARPA
-  123.100.IN-ADDR.ARPA
-  124.100.IN-ADDR.ARPA
-  125.100.IN-ADDR.ARPA
-  126.100.IN-ADDR.ARPA
-  127.100.IN-ADDR.ARPA
-  0.IN-ADDR.ARPA
-  127.IN-ADDR.ARPA
-  254.169.IN-ADDR.ARPA
-  2.0.192.IN-ADDR.ARPA
-  100.51.198.IN-ADDR.ARPA
-  113.0.203.IN-ADDR.ARPA
-  255.255.255.255.IN-ADDR.ARPA
-  0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.IP6.ARPA
-  1.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.IP6.ARPA
-  8.B.D.0.1.0.0.2.IP6.ARPA
-  D.F.IP6.ARPA
-  8.E.F.IP6.ARPA
-  9.E.F.IP6.ARPA
-  A.E.F.IP6.ARPA
-  B.E.F.IP6.ARPA
-  EMPTY.AS112.ARPA
-  HOME.ARPA

Empty zones can be set at the view level and only apply to views of
class IN. Disabled empty zones are only inherited from options if there
are no disabled empty zones specified at the view level. To override the
options list of disabled zones, disable the root zone at the
view level. For example:

::

           disable-empty-zone ".";

If using the address ranges covered here,
reverse zones covering the addresses should already be in place. In practice this
appears to not be the case, with many queries being made to the
infrastructure servers for names in these spaces. So many, in fact, that
sacrificial servers had to be deployed to channel the query load
away from the infrastructure servers.

.. note::

   The real parent servers for these zones should disable all empty zones
   under the parent zone they serve. For the real root servers, this is
   all built-in empty zones. This enables them to return referrals
   to deeper in the tree.

``empty-server``
   This specifies the server name that appears in the returned SOA record for
   empty zones. If none is specified, the zone's name is used.

``empty-contact``
   This specifies the contact name that appears in the returned SOA record for
   empty zones. If none is specified, "." is used.

``empty-zones-enable``
   This enables or disables all empty zones. By default, they are enabled.

``disable-empty-zone``
   This disables individual empty zones. By default, none are disabled. This
   option can be specified multiple times.

.. _content_filtering:

Content Filtering
^^^^^^^^^^^^^^^^^

BIND 9 provides the ability to filter out responses from external
DNS servers containing certain types of data in the answer section.
Specifically, it can reject address (A or AAAA) records if the
corresponding IPv4 or IPv6 addresses match the given
``address_match_list`` of the ``deny-answer-addresses`` option. It can
also reject CNAME or DNAME records if the "alias" name (i.e., the CNAME
alias or the substituted query name due to DNAME) matches the given
``namelist`` of the ``deny-answer-aliases`` option, where "match" means
the alias name is a subdomain of one of the ``name_list`` elements. If
the optional ``namelist`` is specified with ``except-from``, records
whose query name matches the list are accepted regardless of the
filter setting. Likewise, if the alias name is a subdomain of the
corresponding zone, the ``deny-answer-aliases`` filter does not apply;
for example, even if "example.com" is specified for
``deny-answer-aliases``,

::

   www.example.com. CNAME xxx.example.com.

returned by an "example.com" server is accepted.

In the ``address_match_list`` of the ``deny-answer-addresses`` option,
only ``ip_addr`` and ``ip_prefix`` are meaningful; any ``key_id`` is
silently ignored.

If a response message is rejected due to the filtering, the entire
message is discarded without being cached, and a SERVFAIL error is
returned to the client.

This filtering is intended to prevent "DNS rebinding attacks," in which
an attacker, in response to a query for a domain name the attacker
controls, returns an IP address within the user's own network or an alias name
within the user's own domain. A naive web browser or script could then serve
as an unintended proxy, allowing the attacker to get access to an
internal node of the local network that could not be externally accessed
otherwise. See the paper available at
https://dl.acm.org/doi/10.1145/1315245.1315298 for more details
about these attacks.

For example, with a domain named "example.net" and an internal
network using an IPv4 prefix 192.0.2.0/24, an administrator might specify the
following rules:

::

   deny-answer-addresses { 192.0.2.0/24; } except-from { "example.net"; };
   deny-answer-aliases { "example.net"; };

If an external attacker let a web browser in the local network look up
an IPv4 address of "attacker.example.com", the attacker's DNS server
would return a response like this:

::

   attacker.example.com. A 192.0.2.1

in the answer section. Since the rdata of this record (the IPv4 address)
matches the specified prefix 192.0.2.0/24, this response would be
ignored.

On the other hand, if the browser looked up a legitimate internal web
server "www.example.net" and the following response were returned to the
BIND 9 server:

::

   www.example.net. A 192.0.2.2

it would be accepted, since the owner name "www.example.net" matches the
``except-from`` element, "example.net".

Note that this is not really an attack on the DNS per se. In fact, there
is nothing wrong with having an "external" name mapped to an "internal"
IP address or domain name from the DNS point of view; it might actually
be provided for a legitimate purpose, such as for debugging. As long as
the mapping is provided by the correct owner, it either is not possible or does
not make sense to detect whether the intent of the mapping is legitimate
within the DNS. The "rebinding" attack must primarily be
protected at the application that uses the DNS. For a large site,
however, it may be difficult to protect all possible applications at
once. This filtering feature is provided only to help such an
operational environment; turning it on is generally discouraged
unless there is no other choice and the attack is a
real threat to applications.

Care should be particularly taken if using this option for
addresses within 127.0.0.0/8. These addresses are obviously "internal,"
but many applications conventionally rely on a DNS mapping from some
name to such an address. Filtering out DNS records containing this
address spuriously can break such applications.

.. _rpz:

Response Policy Zone (RPZ) Rewriting
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

BIND 9 includes a limited mechanism to modify DNS responses for requests
analogous to email anti-spam DNS rejection lists. Responses can be changed to
deny the existence of domains (NXDOMAIN), deny the existence of IP
addresses for domains (NODATA), or contain other IP addresses or data.

Response policy zones are named in the ``response-policy`` option for
the view, or among the global options if there is no ``response-policy``
option for the view. Response policy zones are ordinary DNS zones
containing RRsets that can be queried normally if allowed. It is usually
best to restrict those queries with something like
``allow-query { localhost; };``. Note that zones using
``masterfile-format map`` cannot be used as policy zones.

A ``response-policy`` option can support multiple policy zones. To
maximize performance, a radix tree is used to quickly identify response
policy zones containing triggers that match the current query. This
imposes an upper limit of 64 on the number of policy zones in a single
``response-policy`` option; more than that is a configuration error.

Rules encoded in response policy zones are processed after those defined in
:ref:`access_control`. All queries from clients which are not permitted access
to the resolver are answered with a status code of REFUSED, regardless of
configured RPZ rules.

Five policy triggers can be encoded in RPZ records.

``RPZ-CLIENT-IP``
   IP records are triggered by the IP address of the DNS client. Client
   IP address triggers are encoded in records that have owner names that
   are subdomains of ``rpz-client-ip``, relativized to the policy zone
   origin name, and that encode an address or address block. IPv4 addresses
   are represented as ``prefixlength.B4.B3.B2.B1.rpz-client-ip``. The
   IPv4 prefix length must be between 1 and 32. All four bytes - B4, B3,
   B2, and B1 - must be present. B4 is the decimal value of the least
   significant byte of the IPv4 address as in IN-ADDR.ARPA.

   IPv6 addresses are encoded in a format similar to the standard IPv6
   text representation,
   ``prefixlength.W8.W7.W6.W5.W4.W3.W2.W1.rpz-client-ip``. Each of
   W8,...,W1 is a one- to four-digit hexadecimal number representing 16
   bits of the IPv6 address as in the standard text representation of
   IPv6 addresses, but reversed as in IP6.ARPA. (Note that this
   representation of IPv6 addresses is different from IP6.ARPA, where each
   hex digit occupies a label.) All 8 words must be present except when
   one set of consecutive zero words is replaced with ``.zz.``, analogous
   to double colons (::) in standard IPv6 text encodings. The IPv6
   prefix length must be between 1 and 128.

``QNAME``
   QNAME policy records are triggered by query names of requests and
   targets of CNAME records resolved to generate the response. The owner
   name of a QNAME policy record is the query name relativized to the
   policy zone.

``RPZ-IP``
   IP triggers are IP addresses in an A or AAAA record in the ANSWER
   section of a response. They are encoded like client-IP triggers,
   except as subdomains of ``rpz-ip``.

``RPZ-NSDNAME``
   NSDNAME triggers match names of authoritative servers for the query name, a
   parent of the query name, a CNAME for the query name, or a parent of a CNAME.
   They are encoded as subdomains of ``rpz-nsdname``, relativized
   to the RPZ origin name.  NSIP triggers match IP addresses in A and AAAA
   RRsets for domains that can be checked against NSDNAME policy records. The
   ``nsdname-enable`` phrase turns NSDNAME triggers off or on for a single
   policy zone or for all zones.

   If authoritative name servers for the query name are not yet known, ``named``
   recursively looks up the authoritative servers for the query name before
   applying an RPZ-NSDNAME rule, which can cause a processing delay.

``RPZ-NSIP``
   NSIP triggers match the IP addresses of authoritative servers. They
   are encoded like IP triggers, except as subdomains of ``rpz-nsip``.
   NSDNAME and NSIP triggers are checked only for names with at least
   ``min-ns-dots`` dots. The default value of ``min-ns-dots`` is 1, to
   exclude top-level domains. The ``nsip-enable`` phrase turns NSIP
   triggers off or on for a single policy zone or for all zones.

   If a name server's IP address is not yet known, ``named``
   recursively looks up the IP address before applying an RPZ-NSIP rule,
   which can cause a processing delay. To speed up processing at the cost
   of precision, the ``nsip-wait-recurse`` option can be used; when set
   to ``no``, RPZ-NSIP rules are only applied when a name server's
   IP address has already been looked up and cached. If a server's IP
   address is not in the cache, the RPZ-NSIP rule is ignored,
   but the address is looked up in the background and the rule
   is applied to subsequent queries. The default is ``yes``,
   meaning RPZ-NSIP rules are always applied, even if an address
   needs to be looked up first.

The query response is checked against all response policy zones, so two
or more policy records can be triggered by a response. Because DNS
responses are rewritten according to at most one policy record, a single
record encoding an action (other than ``DISABLED`` actions) must be
chosen. Triggers, or the records that encode them, are chosen for
rewriting in the following order:

1. Choose the triggered record in the zone that appears first in the
   response-policy option.
2. Prefer CLIENT-IP to QNAME to IP to NSDNAME to NSIP triggers in a
   single zone.
3. Among NSDNAME triggers, prefer the trigger that matches the smallest
   name under the DNSSEC ordering.
4. Among IP or NSIP triggers, prefer the trigger with the longest
   prefix.
5. Among triggers with the same prefix length, prefer the IP or NSIP
   trigger that matches the smallest IP address.

When the processing of a response is restarted to resolve DNAME or CNAME
records and a policy record set has not been triggered, all response
policy zones are again consulted for the DNAME or CNAME names and
addresses.

RPZ record sets are any types of DNS record, except DNAME or DNSSEC, that
encode actions or responses to individual queries. Any of the policies
can be used with any of the triggers. For example, while the
``TCP-only`` policy is commonly used with ``client-IP`` triggers, it can
be used with any type of trigger to force the use of TCP for responses
with owner names in a zone.

``PASSTHRU``
   The auto-acceptance policy is specified by a CNAME whose target is
   ``rpz-passthru``. It causes the response to not be rewritten and is
   most often used to "poke holes" in policies for CIDR blocks.

``DROP``
   The auto-rejection policy is specified by a CNAME whose target is
   ``rpz-drop``. It causes the response to be discarded. Nothing is sent
   to the DNS client.

``TCP-Only``
   The "slip" policy is specified by a CNAME whose target is
   ``rpz-tcp-only``. It changes UDP responses to short, truncated DNS
   responses that require the DNS client to try again with TCP. It is
   used to mitigate distributed DNS reflection attacks.

``NXDOMAIN``
   The "domain undefined" response is encoded by a CNAME whose target is
   the root domain (.).

``NODATA``
   The empty set of resource records is specified by a CNAME whose target
   is the wildcard top-level domain (``*.``). It rewrites the response to
   NODATA or ANCOUNT=0.

``Local Data``
   A set of ordinary DNS records can be used to answer queries. Queries
   for record types not in the set are answered with NODATA.

   A special form of local data is a CNAME whose target is a wildcard
   such as \*.example.com. It is used as if an ordinary CNAME after
   the asterisk (\*) has been replaced with the query name.
   This special form is useful for query logging in the walled garden's
   authoritative DNS server.

All of the actions specified in all of the individual records in a
policy zone can be overridden with a ``policy`` clause in the
``response-policy`` option. An organization using a policy zone provided
by another organization might use this mechanism to redirect domains to
its own walled garden.

``GIVEN``
   The placeholder policy says "do not override but perform the action
   specified in the zone."

``DISABLED``
   The testing override policy causes policy zone records to do nothing
   but log what they would have done if the policy zone were not
   disabled. The response to the DNS query is written (or not)
   according to any triggered policy records that are not disabled.
   Disabled policy zones should appear first, because they are often
   not logged if a higher-precedence trigger is found first.

``PASSTHRU``; ``DROP``; ``TCP-Only``; ``NXDOMAIN``; ``NODATA``
   These settings each override the corresponding per-record policy.

``CNAME domain``
   This causes all RPZ policy records to act as if they were "cname domain"
   records.

By default, the actions encoded in a response policy zone are applied
only to queries that ask for recursion (RD=1). That default can be
changed for a single policy zone, or for all response policy zones in a view,
with a ``recursive-only no`` clause. This feature is useful for serving
the same zone files both inside and outside an :rfc:`1918` cloud and using
RPZ to delete answers that would otherwise contain :rfc:`1918` values on
the externally visible name server or view.

Also by default, RPZ actions are applied only to DNS requests that
either do not request DNSSEC metadata (DO=0) or when no DNSSEC records
are available for the requested name in the original zone (not the response
policy zone). This default can be changed for all response policy zones
in a view with a ``break-dnssec yes`` clause. In that case, RPZ actions
are applied regardless of DNSSEC. The name of the clause option reflects
the fact that results rewritten by RPZ actions cannot verify.

No DNS records are needed for a QNAME or Client-IP trigger; the name or
IP address itself is sufficient, so in principle the query name need not
be recursively resolved. However, not resolving the requested name can
leak the fact that response policy rewriting is in use, and that the name
is listed in a policy zone, to operators of servers for listed names. To
prevent that information leak, by default any recursion needed for a
request is done before any policy triggers are considered. Because
listed domains often have slow authoritative servers, this behavior can
cost significant time. The ``qname-wait-recurse no`` option overrides
the default and enables that behavior when recursion cannot change a
non-error response. The option does not affect QNAME or client-IP
triggers in policy zones listed after other zones containing IP, NSIP,
and NSDNAME triggers, because those may depend on the A, AAAA, and NS
records that would be found during recursive resolution. It also does
not affect DNSSEC requests (DO=1) unless ``break-dnssec yes`` is in use,
because the response would depend on whether RRSIG records were
found during resolution. Using this option can cause error responses
such as SERVFAIL to appear to be rewritten, since no recursion is being
done to discover problems at the authoritative server.

The ``dnsrps-enable yes`` option turns on the DNS Response Policy Service
(DNSRPS) interface, if it has been compiled in ``named`` using
``configure --enable-dnsrps``.

The ``dnsrps-options`` block provides additional RPZ configuration
settings, which are passed through to the DNSRPS provider library.
Multiple DNSRPS settings in an ``dnsrps-options`` string should be
separated with semi-colons (;). The DNSRPS provider, librpz, is passed a
configuration string consisting of the ``dnsrps-options`` text,
concatenated with settings derived from the ``response-policy``
statement.

Note: the ``dnsrps-options`` text should only include configuration
settings that are specific to the DNSRPS provider. For example, the
DNSRPS provider from Farsight Security takes options such as
``dnsrpzd-conf``, ``dnsrpzd-sock``, and ``dnzrpzd-args`` (for details of
these options, see the ``librpz`` documentation). Other RPZ
configuration settings could be included in ``dnsrps-options`` as well,
but if ``named`` were switched back to traditional RPZ by setting
``dnsrps-enable`` to "no", those options would be ignored.

The TTL of a record modified by RPZ policies is set from the TTL of the
relevant record in the policy zone. It is then limited to a maximum value.
The ``max-policy-ttl`` clause changes the maximum number of seconds from its
default of 5. For convenience, TTL-style time-unit suffixes may be used
to specify the value. It also accepts ISO 8601 duration formats.

For example, an administrator might use this option statement:

::

       response-policy { zone "badlist"; };

and this zone statement:

::

       zone "badlist" {type primary; file "primary/badlist"; allow-query {none;}; };

with this zone file:

::

   $TTL 1H
   @                       SOA LOCALHOST. named-mgr.example.com (1 1h 15m 30d 2h)
               NS  LOCALHOST.

   ; QNAME policy records.  There are no periods (.) after the owner names.
   nxdomain.domain.com     CNAME   .               ; NXDOMAIN policy
   *.nxdomain.domain.com   CNAME   .               ; NXDOMAIN policy
   nodata.domain.com       CNAME   *.              ; NODATA policy
   *.nodata.domain.com     CNAME   *.              ; NODATA policy
   bad.domain.com          A       10.0.0.1        ; redirect to a walled garden
               AAAA    2001:2::1
   bzone.domain.com        CNAME   garden.example.com.

   ; do not rewrite (PASSTHRU) OK.DOMAIN.COM
   ok.domain.com           CNAME   rpz-passthru.

   ; redirect x.bzone.domain.com to x.bzone.domain.com.garden.example.com
   *.bzone.domain.com      CNAME   *.garden.example.com.

   ; IP policy records that rewrite all responses containing A records in 127/8
   ;       except 127.0.0.1
   8.0.0.0.127.rpz-ip      CNAME   .
   32.1.0.0.127.rpz-ip     CNAME   rpz-passthru.

   ; NSDNAME and NSIP policy records
   ns.domain.com.rpz-nsdname   CNAME   .
   48.zz.2.2001.rpz-nsip       CNAME   .

   ; auto-reject and auto-accept some DNS clients
   112.zz.2001.rpz-client-ip    CNAME   rpz-drop.
   8.0.0.0.127.rpz-client-ip    CNAME   rpz-drop.

   ; force some DNS clients and responses in the example.com zone to TCP
   16.0.0.1.10.rpz-client-ip   CNAME   rpz-tcp-only.
   example.com                 CNAME   rpz-tcp-only.
   *.example.com               CNAME   rpz-tcp-only.

RPZ can affect server performance. Each configured response policy zone
requires the server to perform one to four additional database lookups
before a query can be answered. For example, a DNS server with four
policy zones, each with all four kinds of response triggers (QNAME, IP,
NSIP, and NSDNAME), requires a total of 17 times as many database lookups
as a similar DNS server with no response policy zones. A BIND 9 server
with adequate memory and one response policy zone with QNAME and IP
triggers might achieve a maximum queries-per-second (QPS) rate about 20%
lower. A server with four response policy zones with QNAME and IP
triggers might have a maximum QPS rate about 50% lower.

Responses rewritten by RPZ are counted in the ``RPZRewrites``
statistics.

The ``log`` clause can be used to optionally turn off rewrite logging
for a particular response policy zone. By default, all rewrites are
logged.

The ``add-soa`` option controls whether the RPZ's SOA record is added to
the section for traceback of changes from this zone.
This can be set at the individual policy zone level or at the
response-policy level. The default is ``yes``.

Updates to RPZ zones are processed asynchronously; if there is more than
one update pending they are bundled together. If an update to a RPZ zone
(for example, via IXFR) happens less than ``min-update-interval``
seconds after the most recent update, the changes are not
carried out until this interval has elapsed. The default is ``60``
seconds. For convenience, TTL-style time-unit suffixes may be used to
specify the value. It also accepts ISO 8601 duration formats.

.. _rrl:

Response Rate Limiting
^^^^^^^^^^^^^^^^^^^^^^

Excessive, almost-identical UDP *responses* can be controlled by
configuring a ``rate-limit`` clause in an ``options`` or ``view``
statement. This mechanism keeps authoritative BIND 9 from being used to
amplify reflection denial-of-service (DoS) attacks. Short BADCOOKIE errors or
truncated (TC=1) responses can be sent to provide rate-limited responses to
legitimate clients within a range of forged, attacked IP addresses.
Legitimate clients react to dropped responses by retrying,
to BADCOOKIE errors by including a server cookie when retrying,
and to truncated responses by switching to TCP.

This mechanism is intended for authoritative DNS servers. It can be used
on recursive servers, but can slow applications such as SMTP servers
(mail receivers) and HTTP clients (web browsers) that repeatedly request
the same domains. When possible, closing "open" recursive servers is
better.

Response rate limiting uses a "credit" or "token bucket" scheme. Each
combination of identical response and client has a conceptual "account"
that earns a specified number of credits every second. A prospective
response debits its account by one. Responses are dropped or truncated
while the account is negative. Responses are tracked within a rolling
window of time which defaults to 15 seconds, but which can be configured with
the ``window`` option to any value from 1 to 3600 seconds (1 hour). The
account cannot become more positive than the per-second limit or more
negative than ``window`` times the per-second limit. When the specified
number of credits for a class of responses is set to 0, those responses
are not rate-limited.

The notions of "identical response" and "DNS client" for rate limiting
are not simplistic. All responses to an address block are counted as if
to a single client. The prefix lengths of address blocks are specified
with ``ipv4-prefix-length`` (default 24) and ``ipv6-prefix-length``
(default 56).

All non-empty responses for a valid domain name (qname) and record type
(qtype) are identical and have a limit specified with
``responses-per-second`` (default 0 or no limit). All valid wildcard
domain names are interpreted as the zone's origin name concatenated to
the "*" name. All empty (NODATA) responses for a valid domain,
regardless of query type, are identical.  Responses in the NODATA class
are limited by ``nodata-per-second`` (default ``responses-per-second``).
Requests for any and all undefined subdomains of a given valid domain
result in NXDOMAIN errors, and are identical regardless of query type.
They are limited by ``nxdomains-per-second`` (default
``responses-per-second``). This controls some attacks using random
names, but can be relaxed or turned off (set to 0) on servers that
expect many legitimate NXDOMAIN responses, such as from anti-spam
rejection lists. Referrals or delegations to the server of a given
domain are identical and are limited by ``referrals-per-second``
(default ``responses-per-second``).

Responses generated from local wildcards are counted and limited as if
they were for the parent domain name. This controls flooding using
random.wild.example.com.

All requests that result in DNS errors other than NXDOMAIN, such as
SERVFAIL and FORMERR, are identical regardless of requested name (qname)
or record type (qtype). This controls attacks using invalid requests or
distant, broken authoritative servers. By default the limit on errors is
the same as the ``responses-per-second`` value, but it can be set
separately with ``errors-per-second``.

Many attacks using DNS involve UDP requests with forged source
addresses. Rate limiting prevents the use of BIND 9 to flood a network
with responses to requests with forged source addresses, but could let a
third party block responses to legitimate requests. There is a mechanism
that can answer some legitimate requests from a client whose address is
being forged in a flood. Setting ``slip`` to 2 (its default) causes
every other UDP request without a valid server cookie to be answered with
a small response. The small size and reduced frequency, and resulting lack of
amplification, of "slipped" responses make them unattractive for
reflection DoS attacks. ``slip`` must be between 0 and 10. A value of 0
does not "slip"; no small responses are sent due to rate limiting. Rather,
all responses are dropped. A value of 1 causes every response to slip;
values between 2 and 10 cause every nth response to slip.

If the request included a client cookie, then a "slipped" response is
a BADCOOKIE error with a server cookie, which allows a legitimate client
to include the server cookie to be exempted from the rate limiting
when it retries the request.
If the request did not include a cookie, then a "slipped" response is
a truncated (TC=1) response, which prompts a legitimate client to
switch to TCP and thus be exempted from the rate limiting. Some error
responses, including REFUSED and SERVFAIL, cannot be replaced with
truncated responses and are instead leaked at the ``slip`` rate.

(Note: dropped responses from an authoritative server may reduce the
difficulty of a third party successfully forging a response to a
recursive resolver. The best security against forged responses is for
authoritative operators to sign their zones using DNSSEC and for
resolver operators to validate the responses. When this is not an
option, operators who are more concerned with response integrity than
with flood mitigation may consider setting ``slip`` to 1, causing all
rate-limited responses to be truncated rather than dropped. This reduces
the effectiveness of rate-limiting against reflection attacks.)

When the approximate query-per-second rate exceeds the ``qps-scale``
value, the ``responses-per-second``, ``errors-per-second``,
``nxdomains-per-second``, and ``all-per-second`` values are reduced by
the ratio of the current rate to the ``qps-scale`` value. This feature
can tighten defenses during attacks. For example, with
``qps-scale 250; responses-per-second 20;`` and a total query rate of
1000 queries/second for all queries from all DNS clients including via
TCP, then the effective responses/second limit changes to (250/1000)*20,
or 5. Responses to requests that included a valid server cookie,
and responses sent via TCP, are not limited but are counted to compute
the query-per-second rate.

Communities of DNS clients can be given their own parameters or no
rate limiting by putting ``rate-limit`` statements in ``view`` statements
instead of in the global ``option`` statement. A ``rate-limit`` statement
in a view replaces, rather than supplements, a ``rate-limit``
statement among the main options. DNS clients within a view can be
exempted from rate limits with the ``exempt-clients`` clause.

UDP responses of all kinds can be limited with the ``all-per-second``
phrase. This rate limiting is unlike the rate limiting provided by
``responses-per-second``, ``errors-per-second``, and
``nxdomains-per-second`` on a DNS server, which are often invisible to
the victim of a DNS reflection attack. Unless the forged requests of the
attack are the same as the legitimate requests of the victim, the
victim's requests are not affected. Responses affected by an
``all-per-second`` limit are always dropped; the ``slip`` value has no
effect. An ``all-per-second`` limit should be at least 4 times as large
as the other limits, because single DNS clients often send bursts of
legitimate requests. For example, the receipt of a single mail message
can prompt requests from an SMTP server for NS, PTR, A, and AAAA records
as the incoming SMTP/TCP/IP connection is considered. The SMTP server
can need additional NS, A, AAAA, MX, TXT, and SPF records as it
considers the SMTP ``Mail From`` command. Web browsers often repeatedly
resolve the same names that are duplicated in HTML <IMG> tags in a page.
``all-per-second`` is similar to the rate limiting offered by firewalls
but is often inferior. Attacks that justify ignoring the contents of DNS
responses are likely to be attacks on the DNS server itself. They
usually should be discarded before the DNS server spends resources making
TCP connections or parsing DNS requests, but that rate limiting must be
done before the DNS server sees the requests.

The maximum size of the table used to track requests and rate-limit
responses is set with ``max-table-size``. Each entry in the table is
between 40 and 80 bytes. The table needs approximately as many entries
as the number of requests received per second. The default is 20,000. To
reduce the cold start of growing the table, ``min-table-size`` (default 500)
can set the minimum table size. Enable ``rate-limit`` category
logging to monitor expansions of the table and inform choices for the
initial and maximum table size.

Use ``log-only yes`` to test rate-limiting parameters without actually
dropping any requests.

Responses dropped by rate limits are included in the ``RateDropped`` and
``QryDropped`` statistics. Responses that are truncated by rate limits are
included in ``RateSlipped`` and ``RespTruncated``.

NXDOMAIN Redirection
^^^^^^^^^^^^^^^^^^^^

``named`` supports NXDOMAIN redirection via two methods:

-  Redirect zone (:ref:`zone_statement_grammar`)
-  Redirect namespace

With either method, when ``named`` gets an NXDOMAIN response it examines a
separate namespace to see if the NXDOMAIN response should be replaced
with an alternative response.

With a redirect zone (``zone "." { type redirect; };``), the data used
to replace the NXDOMAIN is held in a single zone which is not part of
the normal namespace. All the redirect information is contained in the
zone; there are no delegations.

With a redirect namespace (``option { nxdomain-redirect <suffix> };``),
the data used to replace the NXDOMAIN is part of the normal namespace
and is looked up by appending the specified suffix to the original
query name. This roughly doubles the cache required to process
NXDOMAIN responses, as both the original NXDOMAIN response and the
replacement data (or an NXDOMAIN indicating that there is no
replacement) must be stored.

If both a redirect zone and a redirect namespace are configured, the
redirect zone is tried first.

.. _server_statement_grammar:

``server`` Statement Grammar
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. include:: ../misc/server.grammar.rst

.. _server_statement_definition_and_usage:

``server`` Statement Definition and Usage
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ``server`` statement defines characteristics to be associated with a
remote name server. If a prefix length is specified, then a range of
servers is covered. Only the most specific server clause applies,
regardless of the order in ``named.conf``.

The ``server`` statement can occur at the top level of the configuration
file or inside a ``view`` statement. If a ``view`` statement contains
one or more ``server`` statements, only those apply to the view and any
top-level ones are ignored. If a view contains no ``server`` statements,
any top-level ``server`` statements are used as defaults.

If a remote server is giving out bad data, marking it
as bogus prevents further queries to it. The default value of
``bogus`` is ``no``.

The ``provide-ixfr`` clause determines whether the local server, acting
as primary, responds with an incremental zone transfer when the given
remote server, a secondary, requests it. If set to ``yes``, incremental
transfer is provided whenever possible. If set to ``no``, all
transfers to the remote server are non-incremental. If not set, the
value of the ``provide-ixfr`` option in the view or global options block
is used as a default.

The ``request-ixfr`` clause determines whether the local server, acting
as a secondary, requests incremental zone transfers from the given
remote server, a primary. If not set, the value of the ``request-ixfr``
option in the view or global options block is used as a default. It may
also be set in the zone block; if set there, it overrides the
global or view setting for that zone.

IXFR requests to servers that do not support IXFR automatically
fall back to AXFR. Therefore, there is no need to manually list which
servers support IXFR and which ones do not; the global default of
``yes`` should always work. The purpose of the ``provide-ixfr`` and
``request-ixfr`` clauses is to make it possible to disable the use of
IXFR even when both primary and secondary claim to support it: for example, if
one of the servers is buggy and crashes or corrupts data when IXFR is
used.

The ``request-expire`` clause determines whether the local server, when
acting as a secondary, requests the EDNS EXPIRE value. The EDNS EXPIRE
value indicates the remaining time before the zone data expires and
needs to be refreshed. This is used when a secondary server transfers
a zone from another secondary server; when transferring from the
primary, the expiration timer is set from the EXPIRE field of the SOA
record instead. The default is ``yes``.

The ``edns`` clause determines whether the local server attempts to
use EDNS when communicating with the remote server. The default is
``yes``.

The ``edns-udp-size`` option sets the EDNS UDP size that is advertised
by ``named`` when querying the remote server. Valid values are 512 to
4096 bytes; values outside this range are silently adjusted to the
nearest value within it. This option is useful when
advertising a different value to this server than the value advertised
globally: for example, when there is a firewall at the remote site that
is blocking large replies. Note: currently, this sets a single UDP size
for all packets sent to the server; ``named`` does not deviate from this
value. This differs from the behavior of ``edns-udp-size`` in
``options`` or ``view`` statements, where it specifies a maximum value.
The ``server`` statement behavior may be brought into conformance with
the ``options``/``view`` behavior in future releases.

The ``edns-version`` option sets the maximum EDNS VERSION that is
sent to the server(s) by the resolver. The actual EDNS version sent is
still subject to normal EDNS version-negotiation rules (see :rfc:`6891`),
the maximum EDNS version supported by the server, and any other
heuristics that indicate that a lower version should be sent. This
option is intended to be used when a remote server reacts badly to a
given EDNS version or higher; it should be set to the highest version
the remote server is known to support. Valid values are 0 to 255; higher
values are silently adjusted. This option is not needed until
higher EDNS versions than 0 are in use.

The ``max-udp-size`` option sets the maximum EDNS UDP message size
``named`` sends. Valid values are 512 to 4096 bytes; values outside
this range are silently adjusted. This option is useful when
there is a firewall that is blocking large replies from
``named``.

The ``padding`` option adds EDNS Padding options to outgoing messages,
increasing the packet size to a multiple of the specified block size.
Valid block sizes range from 0 (the default, which disables the use of
EDNS Padding) to 512 bytes. Larger values are reduced to 512, with a
logged warning. Note: this option is not currently compatible with no
TSIG or SIG(0), as the EDNS OPT record containing the padding would have
to be added to the packet after it had already been signed.

The ``tcp-only`` option sets the transport protocol to TCP. The default
is to use the UDP transport and to fallback on TCP only when a truncated
response is received.

The ``tcp-keepalive`` option adds EDNS TCP keepalive to messages sent
over TCP. Note that currently idle timeouts in responses are ignored.

The server supports two zone transfer methods. The first,
``one-answer``, uses one DNS message per resource record transferred.
``many-answers`` packs as many resource records as possible into a single
message, which is more efficient.
It is possible to specify which method to use for a server via the
``transfer-format`` option; if not set there, the
``transfer-format`` specified by the ``options`` statement is used.

``transfers`` is used to limit the number of concurrent inbound zone
transfers from the specified server. If no ``transfers`` clause is
specified, the limit is set according to the ``transfers-per-ns``
option.

The ``keys`` clause identifies a ``key_id`` defined by the ``key``
statement, to be used for transaction security (see :ref:`tsig`)
when talking to the remote server. When a request is sent to the remote
server, a request signature is generated using the key specified
here and appended to the message. A request originating from the remote
server is not required to be signed by this key.

Only a single key per server is currently supported.

The ``transfer-source`` and ``transfer-source-v6`` clauses specify the
IPv4 and IPv6 source address, respectively, to be used for zone transfer with the
remote server. For an IPv4 remote server, only
``transfer-source`` can be specified. Similarly, for an IPv6 remote
server, only ``transfer-source-v6`` can be specified. For more details,
see the description of ``transfer-source`` and ``transfer-source-v6`` in
:ref:`zone_transfers`.

The ``notify-source`` and ``notify-source-v6`` clauses specify the IPv4
and IPv6 source address, respectively, to be used for notify messages sent to remote
servers. For an IPv4 remote server, only ``notify-source``
can be specified. Similarly, for an IPv6 remote server, only
``notify-source-v6`` can be specified.

The ``query-source`` and ``query-source-v6`` clauses specify the IPv4
and IPv6 source address, respectively, to be used for queries sent to remote servers.
For an IPv4 remote server, only ``query-source`` can be
specified. Similarly, for an IPv6 remote server, only
``query-source-v6`` can be specified.

The ``request-nsid`` clause determines whether the local server adds
an NSID EDNS option to requests sent to the server. This overrides
``request-nsid`` set at the view or option level.

The ``send-cookie`` clause determines whether the local server adds
a COOKIE EDNS option to requests sent to the server. This overrides
``send-cookie`` set at the view or option level. The ``named`` server
may determine that COOKIE is not supported by the remote server and not
add a COOKIE EDNS option to requests.

.. _statschannels:

``statistics-channels`` Statement Grammar
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. include:: ../misc/statistics-channels.grammar.rst

.. _statistics_channels:

``statistics-channels`` Statement Definition and Usage
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ``statistics-channels`` statement declares communication channels to
be used by system administrators to get access to statistics information
on the name server.

This statement is intended to be flexible to support multiple communication
protocols in the future, but currently only HTTP access is supported. It
requires that BIND 9 be compiled with libxml2 and/or json-c (also known
as libjson0); the ``statistics-channels`` statement is still accepted
even if it is built without the library, but any HTTP access fails
with an error.

An ``inet`` control channel is a TCP socket listening at the specified
``ip_port`` on the specified ``ip_addr``, which can be an IPv4 or IPv6
address. An ``ip_addr`` of ``*`` (asterisk) is interpreted as the IPv4
wildcard address; connections are accepted on any of the system's
IPv4 addresses. To listen on the IPv6 wildcard address, use an
``ip_addr`` of ``::``.

If no port is specified, port 80 is used for HTTP channels. The asterisk
(``*``) cannot be used for ``ip_port``.

Attempts to open a statistics channel are restricted by the
optional ``allow`` clause. Connections to the statistics channel are
permitted based on the ``address_match_list``. If no ``allow`` clause is
present, ``named`` accepts connection attempts from any address. Since
the statistics may contain sensitive internal information, the source of
connection requests must be restricted appropriately so that only
trusted parties can access the statistics channel.

Gathering data exposed by the statistics channel locks various subsystems in
``named``, which could slow down query processing if statistics data is
requested too often.

An issue in the statistics channel would be considered a security issue
only if it could be exploited by unprivileged users circumventing the access
control list. In other words, any issue in the statistics channel that could be
used to access information unavailable otherwise, or to crash ``named``, is
not considered a security issue if it can be avoided through the
use of a secure configuration.

If no ``statistics-channels`` statement is present, ``named`` does not
open any communication channels.

The statistics are available in various formats and views, depending on
the URI used to access them. For example, if the statistics channel is
configured to listen on 127.0.0.1 port 8888, then the statistics are
accessible in XML format at http://127.0.0.1:8888/ or
http://127.0.0.1:8888/xml. A CSS file is included, which can format the
XML statistics into tables when viewed with a stylesheet-capable
browser, and into charts and graphs using the Google Charts API when
using a JavaScript-capable browser.

Broken-out subsets of the statistics can be viewed at
http://127.0.0.1:8888/xml/v3/status (server uptime and last
reconfiguration time), http://127.0.0.1:8888/xml/v3/server (server and
resolver statistics), http://127.0.0.1:8888/xml/v3/zones (zone
statistics), http://127.0.0.1:8888/xml/v3/net (network status and socket
statistics), http://127.0.0.1:8888/xml/v3/mem (memory manager
statistics), http://127.0.0.1:8888/xml/v3/tasks (task manager
statistics), and http://127.0.0.1:8888/xml/v3/traffic (traffic sizes).

The full set of statistics can also be read in JSON format at
http://127.0.0.1:8888/json, with the broken-out subsets at
http://127.0.0.1:8888/json/v1/status (server uptime and last
reconfiguration time), http://127.0.0.1:8888/json/v1/server (server and
resolver statistics), http://127.0.0.1:8888/json/v1/zones (zone
statistics), http://127.0.0.1:8888/json/v1/net (network status and
socket statistics), http://127.0.0.1:8888/json/v1/mem (memory manager
statistics), http://127.0.0.1:8888/json/v1/tasks (task manager
statistics), and http://127.0.0.1:8888/json/v1/traffic (traffic sizes).

.. _trust_anchors:

``trust-anchors`` Statement Grammar
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. include:: ../misc/trust-anchors.grammar.rst

.. _trust-anchors:

``trust-anchors`` Statement Definition and Usage
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ``trust-anchors`` statement defines DNSSEC trust anchors. DNSSEC is
described in :ref:`DNSSEC`.

A trust anchor is defined when the public key or public key digest for a non-authoritative
zone is known but cannot be securely obtained through DNS, either
because it is the DNS root zone or because its parent zone is unsigned.
Once a key or digest has been configured as a trust anchor, it is treated as if it
has been validated and proven secure.

The resolver attempts DNSSEC validation on all DNS data in subdomains of
configured trust anchors. Validation below specified names can be
temporarily disabled by using ``rndc nta``, or permanently disabled with
the ``validate-except`` option.

All keys listed in ``trust-anchors``, and their corresponding zones, are
deemed to exist regardless of what parent zones say. Only keys
configured as trust anchors are used to validate the DNSKEY RRset for
the corresponding name. The parent's DS RRset is not used.

``trust-anchors`` may be set at the top level of ``named.conf`` or within
a view. If it is set in both places, the configurations are additive;
keys defined at the top level are inherited by all views, but keys
defined in a view are only used within that view.

The ``trust-anchors`` statement can contain
multiple trust-anchor entries, each consisting of a
domain name, followed by an "anchor type" keyword indicating
the trust anchor's format, followed by the key or digest data.

If the anchor type is ``static-key`` or
``initial-key``, then it is followed with the
key's flags, protocol, and algorithm, plus the Base64 representation
of the public key data. This is identical to the text
representation of a DNSKEY record.  Spaces, tabs, newlines, and
carriage returns are ignored in the key data, so the
configuration may be split into multiple lines.

If the anchor type is ``static-ds`` or
``initial-ds``, it is followed with the
key tag, algorithm, digest type, and the hexadecimal
representation of the key digest. This is identical to the
text representation of a DS record.  Spaces, tabs, newlines,
and carriage returns are ignored.

Trust anchors configured with the
``static-key`` or ``static-ds``
anchor types are immutable, while keys configured with
``initial-key`` or ``initial-ds``
can be kept up-to-date automatically, without intervention from the resolver operator.
(``static-key`` keys are identical to keys configured using the
deprecated ``trusted-keys`` statement.)

Suppose, for example, that a zone's key-signing key was compromised, and
the zone owner had to revoke and replace the key. A resolver which had
the original key
configured using ``static-key`` or
``static-ds`` would be unable to validate
this zone any longer; it would reply with a SERVFAIL response
code.  This would continue until the resolver operator had
updated the ``trust-anchors`` statement with
the new key.

If, however, the trust anchor had been configured using
``initial-key`` or ``initial-ds``
instead, the zone owner could add a "stand-by" key to
the zone in advance. ``named`` would store
the stand-by key, and when the original key was revoked,
``named`` would be able to transition smoothly
to the new key.  It would also recognize that the old key had
been revoked and cease using that key to validate answers,
minimizing the damage that the compromised key could do.
This is the process used to keep the ICANN root DNSSEC key
up-to-date.

Whereas ``static-key`` and
``static-ds`` trust anchors continue
to be trusted until they are removed from
``named.conf``, an
``initial-key`` or ``initial-ds``
is only trusted *once*: for as long as it
takes to load the managed key database and start the
:rfc:`5011` key maintenance process.

It is not possible to mix static with initial trust anchors
for the same domain name.

The first time ``named`` runs with an
``initial-key`` or ``initial-ds``
configured in ``named.conf``, it fetches the
DNSKEY RRset directly from the zone apex,
and validates it
using the trust anchor specified in ``trust-anchors``.
If the DNSKEY RRset is validly signed by a key matching
the trust anchor, then it is used as the basis for a new
managed-keys database.

From that point on, whenever ``named`` runs, it sees the ``initial-key`` or ``initial-ds``
listed in ``trust-anchors``, checks to make sure :rfc:`5011` key maintenance
has already been initialized for the specified domain, and if so,
simply moves on. The key specified in the ``trust-anchors`` statement is
not used to validate answers; it is superseded by the key or keys stored
in the managed-keys database.

The next time ``named`` runs after an ``initial-key`` or ``initial-ds`` has been *removed*
from the ``trust-anchors`` statement (or changed to a ``static-key`` or ``static-ds``), the
corresponding zone is removed from the managed-keys database, and
:rfc:`5011` key maintenance is no longer used for that domain.

In the current implementation, the managed-keys database is stored as a
master-format zone file.

On servers which do not use views, this file is named
``managed-keys.bind``. When views are in use, there is a separate
managed-keys database for each view; the filename is the view name
(or, if a view name contains characters which would make it illegal as a
filename, a hash of the view name), followed by the suffix ``.mkeys``.

When the key database is changed, the zone is updated. As with any other
dynamic zone, changes are written into a journal file, e.g.,
``managed-keys.bind.jnl`` or ``internal.mkeys.jnl``. Changes are
committed to the primary file as soon as possible afterward,
usually within 30 seconds. Whenever ``named`` is using
automatic key maintenance, the zone file and journal file can be
expected to exist in the working directory. (For this reason, among
others, the working directory should be always be writable by
``named``.)

If the ``dnssec-validation`` option is set to ``auto``, ``named``
automatically initializes an ``initial-key`` for the root zone. The key
that is used to initialize the key-maintenance process is stored in
``bind.keys``; the location of this file can be overridden with the
``bindkeys-file`` option. As a fallback in the event no ``bind.keys``
can be found, the initializing key is also compiled directly into
``named``.

.. _dnssec_policy_grammar:

``dnssec-policy`` Statement Grammar
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. include:: ../misc/dnssec-policy.grammar.rst

.. _dnssec_policy:

``dnssec-policy`` Statement Definition and Usage
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ``dnssec-policy`` statement defines a key and signing policy (KASP)
for zones.

A KASP determines how one or more zones are signed with DNSSEC.  For
example, it specifies how often keys should roll, which cryptographic
algorithms to use, and how often RRSIG records need to be refreshed.

Keys are not shared among zones, which means that one set of keys per
zone is generated even if they have the same policy.  If multiple views
are configured with different versions of the same zone, each separate
version uses the same set of signing keys.

Multiple key and signing policies can be configured.  To attach a policy
to a zone, add a ``dnssec-policy`` option to the ``zone`` statement,
specifying the name of the policy that should be used.

The ``dnssec-policy`` statement requires dynamic DNS to be set up, or
``inline-signing`` to be enabled.

If ``inline-signing`` is enabled, this means that a signed version of the
zone is maintained separately and is written out to a different file on disk
(the zone's filename plus a ``.signed`` extension).

If the zone is dynamic because it is configured with an ``update-policy`` or
``allow-update``, the DNSSEC records are written to the filename set in the
original zone's ``file``, unless ``inline-signing`` is explicitly set.

Key rollover timing is computed for each key according to the key
lifetime defined in the KASP.  The lifetime may be modified by zone TTLs
and propagation delays, to prevent validation failures.  When a key
reaches the end of its lifetime, ``named`` generates and publishes a new
key automatically, then deactivates the old key and activates the new
one; finally, the old key is retired according to a computed schedule.

Zone-signing key (ZSK) rollovers require no operator input.  Key-signing
key (KSK) and combined-signing key (CSK) rollovers require action to be
taken to submit a DS record to the parent.  Rollover timing for KSKs and
CSKs is adjusted to take into account delays in processing and
propagating DS updates.

There are two predefined ``dnssec-policy`` names: ``none`` and
``default``.  Setting a zone's policy to ``none`` is the same as not
setting ``dnssec-policy`` at all; the zone is not signed.  Policy
``default`` causes the zone to be signed with a single combined-signing
key (CSK) using algorithm ECDSAP256SHA256; this key has an unlimited
lifetime.  (A verbose copy of this policy may be found in the source
tree, in the file ``doc/misc/dnssec-policy.default.conf``.)

.. note:: The default signing policy may change in future releases.
   This could require changes to a signing policy when upgrading to a
   new version of BIND.  Check the release notes carefully when
   upgrading to be informed of such changes.  To prevent policy changes
   on upgrade, use an explicitly defined ``dnssec-policy``, rather than
   ``default``.

If a ``dnssec-policy`` statement is modified and the server restarted or
reconfigured, ``named`` attempts to change the policy smoothly from the
old one to the new.  For example, if the key algorithm is changed, then
a new key is generated with the new algorithm, and the old algorithm is
retired when the existing key's lifetime ends.

.. note:: Rolling to a new policy while another key rollover is already
   in progress is not yet supported, and may result in unexpected
   behavior.

The following options can be specified in a ``dnssec-policy`` statement:

  ``dnskey-ttl``
    This indicates the TTL to use when generating DNSKEY resource
    records.  The default is 1 hour (3600 seconds).

  ``keys``
    This is a list specifying the algorithms and roles to use when
    generating keys and signing the zone.  Entries in this list do not
    represent specific DNSSEC keys, which may be changed on a regular
    basis, but the roles that keys play in the signing policy.  For
    example, configuring a KSK of algorithm RSASHA256 ensures that the
    DNSKEY RRset always includes a key-signing key for that algorithm.

    Here is an example (for illustration purposes only) of some possible
    entries in a ``keys`` list:

    ::

        keys {
            ksk key-directory lifetime unlimited algorithm rsasha256 2048;
            zsk lifetime P30D algorithm 8;
            csk lifetime P6MT12H3M15S algorithm ecdsa256;
        };

    This example specifies that three keys should be used in the zone.
    The first token determines which role the key plays in signing
    RRsets.  If set to ``ksk``, then this is a key-signing key; it has
    the KSK flag set and is only used to sign DNSKEY, CDS, and CDNSKEY
    RRsets.  If set to ``zsk``, this is a zone-signing key; the KSK flag
    is unset, and the key signs all RRsets *except* DNSKEY, CDS, and
    CDNSKEY.  If set to ``csk``, the key has the KSK flag set and is
    used to sign all RRsets.

    An optional second token determines where the key is stored.
    Currently, keys can only be stored in the configured
    ``key-directory``.  This token may be used in the future to store
    keys in hardware security modules or separate directories.

    The ``lifetime`` parameter specifies how long a key may be used
    before rolling over.  In the example above, the first key has an
    unlimited lifetime, the second key may be used for 30 days, and the
    third key has a rather peculiar lifetime of 6 months, 12 hours, 3
    minutes, and 15 seconds.  A lifetime of 0 seconds is the same as
    ``unlimited``.

    Note that the lifetime of a key may be extended if retiring it too
    soon would cause validation failures.  For example, if the key were
    configured to roll more frequently than its own TTL, its lifetime
    would automatically be extended to account for this.

    The ``algorithm`` parameter specifies the key's algorithm, expressed
    either as a string ("rsasha256", "ecdsa384", etc.) or as a decimal
    number.  An optional second parameter specifies the key's size in
    bits.  If it is omitted, as shown in the example for the second and
    third keys, an appropriate default size for the algorithm is used.
    Each KSK/ZSK pair must have the same algorithm. A CSK combines the
    functionality of a ZSK and a KSK.

  ``purge-keys``
    This is the time after when DNSSEC keys that have been deleted from
    the zone can be removed from disk. If a key still determined to have
    presence (for example in some resolver cache), ``named`` will not
    remove the key files.

    The default is ``P90D`` (90 days). Set this option to ``0`` to never
    purge deleted keys.

  ``publish-safety``
    This is a margin that is added to the pre-publication interval in
    rollover timing calculations, to give some extra time to cover
    unforeseen events.  This increases the time between when keys are
    published and when they become active.  The default is ``PT1H`` (1
    hour).

  ``retire-safety``
    This is a margin that is added to the post-publication interval in
    rollover timing calculations, to give some extra time to cover
    unforeseen events.  This increases the time a key remains published
    after it is no longer active.  The default is ``PT1H`` (1 hour).

  ``signatures-refresh``
    This determines how frequently an RRSIG record needs to be
    refreshed.  The signature is renewed when the time until the
    expiration time is less than the specified interval.  The default is
    ``P5D`` (5 days), meaning signatures that expire in 5 days or sooner
    are refreshed.

  ``signatures-validity``
    This indicates the validity period of an RRSIG record (subject to
    inception offset and jitter).  The default is ``P2W`` (2 weeks).

  ``signatures-validity-dnskey``
    This is similar to ``signatures-validity``, but for DNSKEY records.
    The default is ``P2W`` (2 weeks).

  ``max-zone-ttl``

   This specifies the maximum permissible TTL value for the zone.  When
   a zone file is loaded, any record encountered with a TTL higher than
   ``max-zone-ttl`` causes the zone to be rejected.

   This ensures that when rolling to a new DNSKEY, the old key will remain
   available until RRSIG records have expired from caches. The
   ``max-zone-ttl`` option guarantees that the largest TTL in the
   zone is no higher than a known and predictable value.

    .. note:: Because ``map``-format files load directly into memory,
       this option cannot be used with them.

   The default value ``PT24H`` (24 hours).  A value of zero is treated
   as if the default value were in use.


  ``nsec3param``
    Use NSEC3 instead of NSEC, and optionally set the NSEC3 parameters.

    Here is an example of an ``nsec3`` configuration:

    ::

        nsec3param iterations 5 optout no salt-length 8;

    The default is to use NSEC. The ``iterations``, ``optout``, and
    ``salt-length`` parts are optional, but if not set, the values in
    the example above are the default NSEC3 parameters. Note that the
    specific salt string is not specified by the user; :iscman:`named` creates a salt
    of the indicated length.

    .. warning::
       Do not use extra :term:`iterations <Iterations>`, :term:`salt <Salt>`, and
       :term:`opt-out <Opt-out>` unless their implications are fully understood.
       A higher number of iterations causes interoperability problems and opens
       servers to CPU-exhausting DoS attacks.

  ``zone-propagation-delay``
    This is the expected propagation delay from the time when a zone is
    first updated to the time when the new version of the zone is served
    by all secondary servers.  The default is ``PT5M`` (5 minutes).

  ``parent-ds-ttl``
    This is the TTL of the DS RRset that the parent zone uses.  The
    default is ``P1D`` (1 day).

  ``parent-propagation-delay``
    This is the expected propagation delay from the time when the parent
    zone is updated to the time when the new version is served by all of
    the parent zone's name servers.  The default is ``PT1H`` (1 hour).

Automated KSK Rollovers
^^^^^^^^^^^^^^^^^^^^^^^

BIND has mechanisms in place to facilitate automated KSK rollovers. It
publishes CDS and CDNSKEY records that can be used by the parent zone to
publish or withdraw the zone's DS records. BIND will query the parental
agents to see if the new DS is actually published before withdrawing the
old DNSSEC key.

   .. note::
      The DS response is not validated so it is recommended to set up a
      trust relationship with the parental agent. For example, use TSIG to
      authenticate the parental agent, or point to a validating resolver.

The following options apply to DS queries sent to ``parental-agents``:

``parental-source``
   ``parental-source`` determines which local source address, and optionally
   UDP port, is used to send parental DS queries. This statement sets the
   ``parental-source`` for all zones, but can be overridden on a per-zone or
   per-view basis by including a ``parental-source`` statement within the
   ``zone`` or ``view`` block in the configuration file.

   .. warning:: Specifying a single port is discouraged, as it removes a layer of
      protection against spoofing errors.

   .. warning:: The configured ``port`` must not be same as the listening port.

``parental-source-v6``
   This option acts like ``parental-source``, but applies to parental DS
   queries sent to IPv6 addresses.

.. _managed-keys:

``managed-keys`` Statement Grammar
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. include:: ../misc/managed-keys.grammar.rst

.. _managed_keys:

``managed-keys`` Statement Definition and Usage
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ``managed-keys`` statement has been
deprecated in favor of :ref:`trust_anchors`
with the ``initial-key`` keyword.

.. _trusted-keys:

``trusted-keys`` Statement Grammar
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. include:: ../misc/trusted-keys.grammar.rst

.. _trusted_keys:

``trusted-keys`` Statement Definition and Usage
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ``trusted-keys`` statement has been deprecated in favor of
:ref:`trust_anchors` with the ``static-key`` keyword.

.. _view_statement_grammar:

``view`` Statement Grammar
~~~~~~~~~~~~~~~~~~~~~~~~~~

::

   view view_name [ class ] {
       match-clients { address_match_list } ;
       match-destinations { address_match_list } ;
       match-recursive-only yes_or_no ;
     [ view_option ; ... ]
     [ zone_statement ; ... ]
   } ;

.. _view_statement:

``view`` Statement Definition and Usage
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ``view`` statement is a powerful feature of BIND 9 that lets a name
server answer a DNS query differently depending on who is asking. It is
particularly useful for implementing split DNS setups without having to
run multiple servers.

Each ``view`` statement defines a view of the DNS namespace that is
seen by a subset of clients. A client matches a view if its source IP
address matches the ``address_match_list`` of the view's
``match-clients`` clause, and its destination IP address matches the
``address_match_list`` of the view's ``match-destinations`` clause. If
not specified, both ``match-clients`` and ``match-destinations`` default
to matching all addresses. In addition to checking IP addresses,
``match-clients`` and ``match-destinations`` can also take ``keys``
which provide an mechanism for the client to select the view. A view can
also be specified as ``match-recursive-only``, which means that only
recursive requests from matching clients match that view. The order
of the ``view`` statements is significant; a client request is
resolved in the context of the first ``view`` that it matches.

Zones defined within a ``view`` statement are only accessible to
clients that match the ``view``. By defining a zone of the same name in
multiple views, different zone data can be given to different clients:
for example, "internal" and "external" clients in a split DNS setup.

Many of the options given in the ``options`` statement can also be used
within a ``view`` statement, and then apply only when resolving queries
with that view. When no view-specific value is given, the value in the
``options`` statement is used as a default. Also, zone options can have
default values specified in the ``view`` statement; these view-specific
defaults take precedence over those in the ``options`` statement.

Views are class-specific. If no class is given, class IN is assumed.
Note that all non-IN views must contain a hint zone, since only the IN
class has compiled-in default hints.

If there are no ``view`` statements in the config file, a default view
that matches any client is automatically created in class IN. Any
``zone`` statements specified on the top level of the configuration file
are considered to be part of this default view, and the ``options``
statement applies to the default view. If any explicit ``view``
statements are present, all ``zone`` statements must occur inside
``view`` statements.

Here is an example of a typical split DNS setup implemented using
``view`` statements:

::

   view "internal" {
         // This should match our internal networks.
         match-clients { 10.0.0.0/8; };

         // Provide recursive service to internal
         // clients only.
         recursion yes;

         // Provide a complete view of the example.com
         // zone including addresses of internal hosts.
         zone "example.com" {
           type primary;
           file "example-internal.db";
         };
   };

   view "external" {
         // Match all clients not matched by the
         // previous view.
         match-clients { any; };

         // Refuse recursive service to external clients.
         recursion no;

         // Provide a restricted view of the example.com
         // zone containing only publicly accessible hosts.
         zone "example.com" {
          type primary;
          file "example-external.db";
         };
   };

.. _zone_statement_grammar:

``zone`` Statement Grammar
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. include:: ../misc/master.zoneopt.rst
.. include:: ../misc/slave.zoneopt.rst
.. include:: ../misc/mirror.zoneopt.rst
.. include:: ../misc/hint.zoneopt.rst
.. include:: ../misc/stub.zoneopt.rst
.. include:: ../misc/static-stub.zoneopt.rst
.. include:: ../misc/forward.zoneopt.rst
.. include:: ../misc/redirect.zoneopt.rst
.. include:: ../misc/delegation-only.zoneopt.rst
.. include:: ../misc/in-view.zoneopt.rst

.. _zone_statement:

``zone`` Statement Definition and Usage
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. _zone_types:

Zone Types
^^^^^^^^^^

The ``type`` keyword is required for the ``zone`` configuration unless
it is an ``in-view`` configuration. Its acceptable values are:
``primary`` (or ``master``), ``secondary`` (or ``slave``), ``mirror``,
``hint``, ``stub``, ``static-stub``, ``forward``, ``redirect``,
or ``delegation-only``.

``primary``
   A primary zone has a master copy of the data for the zone and is able
   to provide authoritative answers for it. Type ``master`` is a synonym
   for ``primary``.

``secondary``
    A secondary zone is a replica of a primary zone. Type ``slave`` is a
    synonym for ``secondary``. The ``primaries`` list specifies one or more IP
    addresses of primary servers that the secondary contacts to update
    its copy of the zone.  Primaries list elements can
    also be names of other primaries lists.  By default,
    transfers are made from port 53 on the servers;
    this can be changed for all servers by specifying
    a port number before the list of IP addresses,
    or on a per-server basis after the IP address.
    Authentication to the primary can also be done with
    per-server TSIG keys.  If a file is specified, then the
    replica is written to this file
    whenever the zone
    is changed, and reloaded from this file on a server
    restart. Use of a file is recommended, since it
    often speeds server startup and eliminates a
    needless waste of bandwidth. Note that for large
    numbers (in the tens or hundreds of thousands) of
    zones per server, it is best to use a two-level
    naming scheme for zone filenames. For example,
    a secondary server for the zone
    ``example.com`` might place
    the zone contents into a file called
    ``ex/example.com``, where
    ``ex/`` is just the first two
    letters of the zone name. (Most operating systems
    behave very slowly if there are 100000 files in a single directory.)

``mirror``
   A mirror zone is similar to a zone of type ``secondary``, except its
   data is subject to DNSSEC validation before being used in answers.
   Validation is applied to the entire zone during the zone transfer
   process, and again when the zone file is loaded from disk upon
   restarting ``named``. If validation of a new version of a mirror zone
   fails, a retransfer is scheduled; in the meantime, the most recent
   correctly validated version of that zone is used until it either
   expires or a newer version validates correctly. If no usable zone
   data is available for a mirror zone, due to either transfer failure
   or expiration, traditional DNS recursion is used to look up the
   answers instead. Mirror zones cannot be used in a view that does not
   have recursion enabled.

   Answers coming from a mirror zone look almost exactly like answers
   from a zone of type ``secondary``, with the notable exceptions that
   the AA bit ("authoritative answer") is not set, and the AD bit
   ("authenticated data") is.

   Mirror zones are intended to be used to set up a fast local copy of
   the root zone (see :rfc:`8806`). A default list of primary servers
   for the IANA root zone is built into ``named``, so its mirroring can
   be enabled using the following configuration:

   ::

       zone "." {
           type mirror;
       };

   Mirror zone validation always happens for the entire zone contents.
   This ensures that each version of the zone used by the resolver is
   fully self-consistent with respect to DNSSEC. For incoming mirror
   zone IXFRs, every revision of the zone contained in the IXFR sequence
   is validated independently, in the order in which the zone revisions
   appear on the wire. For this reason, it might be useful to force use
   of AXFR for mirror zones by setting ``request-ixfr no;`` for the
   relevant zone (or view). Other, more efficient zone verification
   methods may be added in the future.

   To make mirror zone contents persist between ``named`` restarts, use
   the :ref:`file <file-option>` option.

   Mirroring a zone other than root requires an explicit list of primary
   servers to be provided using the ``primaries`` option (see
   :ref:`primaries_grammar` for details), and a key-signing key (KSK)
   for the specified zone to be explicitly configured as a trust anchor
   (see :ref:`trust-anchors`).

   When configuring NOTIFY for a mirror zone, only ``notify no;`` and
   ``notify explicit;`` can be used at the zone level; any other
   ``notify`` setting at the zone level is a configuration error. Using
   any other ``notify`` setting at the ``options`` or ``view`` level
   causes that setting to be overridden with ``notify explicit;`` for
   the mirror zone. The global default for the ``notify`` option is
   ``yes``, so mirror zones are by default configured with ``notify
   explicit;``.

   Outgoing transfers of mirror zones are disabled by default but may be
   enabled using :ref:`allow-transfer <allow-transfer-access>`.

   .. note::
      Use of this zone type with any zone other than the root should be
      considered *experimental* and may cause performance issues,
      especially for zones that are large and/or frequently updated.

``hint``
   The initial set of root name servers is specified using a hint zone.
   When the server starts, it uses the root hints to find a root name
   server and get the most recent list of root name servers. If no hint zone
   is specified for class IN, the server uses a compiled-in default set of
   root servers hints. Classes other than IN have no built-in default hints.

``stub``
   A stub zone is similar to a secondary zone, except that it replicates only
   the NS records of a primary zone instead of the entire zone. Stub zones
   are not a standard part of the DNS; they are a feature specific to the
   BIND implementation.

   Stub zones can be used to eliminate the need for a glue NS record in a parent
   zone, at the expense of maintaining a stub zone entry and a set of name
   server addresses in ``named.conf``. This usage is not recommended for
   new configurations, and BIND 9 supports it only in a limited way. If a BIND 9
   primary, serving a parent zone, has child stub
   zones configured, all the secondary servers for the parent zone also need to
   have the same child stub zones configured.

   Stub zones can also be used as a way to force the resolution of a given
   domain to use a particular set of authoritative servers. For example, the
   caching name servers on a private network using :rfc:`1918` addressing may be
   configured with stub zones for ``10.in-addr.arpa`` to use a set of
   internal name servers as the authoritative servers for that domain.

``static-stub``
   A static-stub zone is similar to a stub zone, with the following
   exceptions: the zone data is statically configured, rather than
   transferred from a primary server; and when recursion is necessary for a query
   that matches a static-stub zone, the locally configured data (name server
   names and glue addresses) is always used, even if different authoritative
   information is cached.

   Zone data is configured via the ``server-addresses`` and ``server-names``
   zone options.

   The zone data is maintained in the form of NS and (if necessary) glue A or
   AAAA RRs internally, which can be seen by dumping zone databases with
   ``rndc dumpdb -all``. The configured RRs are considered local configuration
   parameters rather than public data. Non-recursive queries (i.e., those
   with the RD bit off) to a static-stub zone are therefore prohibited and
   are responded to with REFUSED.

   Since the data is statically configured, no zone maintenance action takes
   place for a static-stub zone. For example, there is no periodic refresh
   attempt, and an incoming notify message is rejected with an rcode
   of NOTAUTH.

   Each static-stub zone is configured with internally generated NS and (if
   necessary) glue A or AAAA RRs.

``forward``
   A forward zone is a way to configure forwarding on a per-domain basis.
   A ``zone`` statement of type ``forward`` can contain a ``forward`` and/or
   ``forwarders`` statement, which applies to queries within the domain
   given by the zone name. If no ``forwarders`` statement is present, or an
   empty list for ``forwarders`` is given, then no forwarding is done
   for the domain, canceling the effects of any forwarders in the ``options``
   statement. Thus, to use this type of zone to change the
   behavior of the global ``forward`` option (that is, "forward first" to,
   then "forward only", or vice versa), but use the same servers as set
   globally, re-specify the global forwarders.

``redirect``
   Redirect zones are used to provide answers to queries when normal
   resolution would result in NXDOMAIN being returned. Only one redirect zone
   is supported per view. ``allow-query`` can be used to restrict which
   clients see these answers.

   If the client has requested DNSSEC records (DO=1) and the NXDOMAIN response
   is signed, no substitution occurs.

   To redirect all NXDOMAIN responses to 100.100.100.2 and
   2001:ffff:ffff::100.100.100.2, configure a type ``redirect`` zone
   named ".", with the zone file containing wildcard records that point to
   the desired addresses: ``*. IN A 100.100.100.2`` and
   ``*. IN AAAA 2001:ffff:ffff::100.100.100.2``.

   As another example, to redirect all Spanish names (under .ES), use similar entries
   but with the names ``*.ES.`` instead of ``*.``. To redirect all commercial
   Spanish names (under COM.ES), use wildcard entries
   called ``*.COM.ES.``.

   Note that the redirect zone supports all possible types; it is not
   limited to A and AAAA records.

   If a redirect zone is configured with a ``primaries`` option, then it is
   transferred in as if it were a secondary zone. Otherwise, it is loaded from a
   file as if it were a primary zone.

   Because redirect zones are not referenced directly by name, they are not
   kept in the zone lookup table with normal primary and secondary zones. To reload
   a redirect zone, use ``rndc reload -redirect``; to retransfer a
   redirect zone configured as a secondary, use ``rndc retransfer -redirect``.
   When using ``rndc reload`` without specifying a zone name, redirect
   zones are reloaded along with other zones.

``delegation-only``
   This zone type is used to enforce the delegation-only status of infrastructure
   zones (e.g., COM, NET, ORG). Any answer that is received without an
   explicit or implicit delegation in the authority section is treated
   as NXDOMAIN. This does not apply to the zone apex, and should not be
   applied to leaf zones.

   ``delegation-only`` has no effect on answers received from forwarders.

   See caveats in :ref:`root-delegation-only <root-delegation-only>`.

``in-view``
   When using multiple views, a ``primary`` or ``secondary`` zone configured
   in one view can be referenced in a subsequent view. This allows both views
   to use the same zone without the overhead of loading it more than once. This
   is configured using a ``zone`` statement, with an ``in-view`` option
   specifying the view in which the zone is defined. A ``zone`` statement
   containing ``in-view`` does not need to specify a type, since that is part
   of the zone definition in the other view.

   See :ref:`multiple_views` for more information.

Class
^^^^^

The zone's name may optionally be followed by a class. If a class is not
specified, class ``IN`` (for ``Internet``) is assumed. This is correct
for the vast majority of cases.

The ``hesiod`` class is named for an information service from MIT's
Project Athena. It was used to share information about various systems
databases, such as users, groups, printers, and so on. The keyword ``HS``
is a synonym for hesiod.

Another MIT development is Chaosnet, a LAN protocol created in the
mid-1970s. Zone data for it can be specified with the ``CHAOS`` class.

.. _zone_options:

Zone Options
^^^^^^^^^^^^

``allow-notify``
   See the description of ``allow-notify`` in :ref:`access_control`.

``allow-query``
   See the description of ``allow-query`` in :ref:`access_control`.

``allow-query-on``
   See the description of ``allow-query-on`` in :ref:`access_control`.

``allow-transfer``
   See the description of ``allow-transfer`` in :ref:`access_control`.

``allow-update``
   See the description of ``allow-update`` in :ref:`access_control`.

``update-policy``
   This specifies a "Simple Secure Update" policy. See :ref:`dynamic_update_policies`.

``allow-update-forwarding``
   See the description of ``allow-update-forwarding`` in :ref:`access_control`.

``also-notify``
   This option is only meaningful if ``notify`` is active for this zone. The set of
   machines that receive a ``DNS NOTIFY`` message for this zone is
   made up of all the listed name servers (other than the primary)
   for the zone, plus any IP addresses specified with
   ``also-notify``. A port may be specified with each ``also-notify``
   address to send the notify messages to a port other than the default
   of 53. A TSIG key may also be specified to cause the ``NOTIFY`` to be
   signed by the given key. ``also-notify`` is not meaningful for stub
   zones. The default is the empty list.

``check-names``
   This option is used to restrict the character set and syntax of
   certain domain names in primary files and/or DNS responses received
   from the network. The default varies according to zone type. For
   ``primary`` zones the default is ``fail``; for ``secondary`` zones the
   default is ``warn``. It is not implemented for ``hint`` zones.

``check-mx``
   See the description of ``check-mx`` in :ref:`boolean_options`.

``check-spf``
   See the description of ``check-spf`` in :ref:`boolean_options`.

``check-wildcard``
   See the description of ``check-wildcard`` in :ref:`boolean_options`.

``check-integrity``
   See the description of ``check-integrity`` in :ref:`boolean_options`.

``check-sibling``
   See the description of ``check-sibling`` in :ref:`boolean_options`.

``zero-no-soa-ttl``
   See the description of ``zero-no-soa-ttl`` in :ref:`boolean_options`.

``update-check-ksk``
   See the description of ``update-check-ksk`` in :ref:`boolean_options`.

``dnssec-loadkeys-interval``
   See the description of ``dnssec-loadkeys-interval`` in :ref:`options`.

``dnssec-update-mode``
   See the description of ``dnssec-update-mode`` in :ref:`options`.

``dnssec-dnskey-kskonly``
   See the description of ``dnssec-dnskey-kskonly`` in :ref:`boolean_options`.

``try-tcp-refresh``
   See the description of ``try-tcp-refresh`` in :ref:`boolean_options`.

``database``
   This specifies the type of database to be used to store the zone data.
   The string following the ``database`` keyword is interpreted as a
   list of whitespace-delimited words. The first word identifies the
   database type, and any subsequent words are passed as arguments to
   the database to be interpreted in a way specific to the database
   type.

   The default is ``rbt``, BIND 9's native in-memory red-black tree
   database. This database does not take arguments.

   Other values are possible if additional database drivers have been
   linked into the server. Some sample drivers are included with the
   distribution but none are linked in by default.

``dialup``
   See the description of ``dialup`` in :ref:`boolean_options`.

``delegation-only``
   This flag only applies to forward, hint, and stub zones. If set to
   ``yes``, then the zone is treated as if it is also a
   delegation-only type zone.

   See caveats in :ref:`root-delegation-only <root-delegation-only>`.

.. _file-option:

``file``
   This sets the zone's filename. In ``primary``, ``hint``, and ``redirect``
   zones which do not have ``primaries`` defined, zone data is loaded from
   this file. In ``secondary``, ``mirror``, ``stub``, and ``redirect`` zones
   which do have ``primaries`` defined, zone data is retrieved from
   another server and saved in this file. This option is not applicable
   to other zone types.

``forward``
   This option is only meaningful if the zone has a forwarders list. The ``only`` value
   causes the lookup to fail after trying the forwarders and getting no
   answer, while ``first`` allows a normal lookup to be tried.

``forwarders``
   This is used to override the list of global forwarders. If it is not
   specified in a zone of type ``forward``, no forwarding is done for
   the zone and the global options are not used.

``journal``
   This allows the default journal's filename to be overridden. The default is
   the zone's filename with "``.jnl``" appended. This is applicable to
   ``primary`` and ``secondary`` zones.

``max-ixfr-ratio``
   See the description of ``max-ixfr-ratio`` in :ref:`options`.

``max-journal-size``
   See the description of ``max-journal-size`` in :ref:`server_resource_limits`.

``max-records``
   See the description of ``max-records`` in :ref:`server_resource_limits`.

``max-transfer-time-in``
   See the description of ``max-transfer-time-in`` in :ref:`zone_transfers`.

``max-transfer-idle-in``
   See the description of ``max-transfer-idle-in`` in :ref:`zone_transfers`.

``max-transfer-time-out``
   See the description of ``max-transfer-time-out`` in :ref:`zone_transfers`.

``max-transfer-idle-out``
   See the description of ``max-transfer-idle-out`` in :ref:`zone_transfers`.

``notify``
   See the description of ``notify`` in :ref:`boolean_options`.

``notify-delay``
   See the description of ``notify-delay`` in :ref:`tuning`.

``notify-to-soa``
   See the description of ``notify-to-soa`` in :ref:`boolean_options`.

``zone-statistics``
   See the description of ``zone-statistics`` in :ref:`options`.

``server-addresses``
   This option is only meaningful for static-stub zones. This is a list of IP addresses
   to which queries should be sent in recursive resolution for the zone.
   A non-empty list for this option internally configures the apex
   NS RR with associated glue A or AAAA RRs.

   For example, if "example.com" is configured as a static-stub zone
   with 192.0.2.1 and 2001:db8::1234 in a ``server-addresses`` option,
   the following RRs are internally configured:

   ::

      example.com. NS example.com.
      example.com. A 192.0.2.1
      example.com. AAAA 2001:db8::1234

   These records are used internally to resolve names under the
   static-stub zone. For instance, if the server receives a query for
   "www.example.com" with the RD bit on, the server initiates
   recursive resolution and sends queries to 192.0.2.1 and/or
   2001:db8::1234.

``server-names``
   This option is only meaningful for static-stub zones. This is a list of domain names
   of name servers that act as authoritative servers of the static-stub
   zone. These names are resolved to IP addresses when ``named``
   needs to send queries to these servers. For this supplemental
   resolution to be successful, these names must not be a subdomain of the
   origin name of the static-stub zone. That is, when "example.net" is the
   origin of a static-stub zone, "ns.example" and "master.example.com"
   can be specified in the ``server-names`` option, but "ns.example.net"
   cannot; it is rejected by the configuration parser.

   A non-empty list for this option internally configures the apex
   NS RR with the specified names. For example, if "example.com" is
   configured as a static-stub zone with "ns1.example.net" and
   "ns2.example.net" in a ``server-names`` option, the following RRs
   are internally configured:

   ::

      example.com. NS ns1.example.net.
      example.com. NS ns2.example.net.

   These records are used internally to resolve names under the
   static-stub zone. For instance, if the server receives a query for
   "www.example.com" with the RD bit on, the server initiates recursive
   resolution, resolves "ns1.example.net" and/or "ns2.example.net" to IP
   addresses, and then sends queries to one or more of these addresses.

``sig-validity-interval``
   See the description of ``sig-validity-interval`` in :ref:`tuning`.

``sig-signing-nodes``
   See the description of ``sig-signing-nodes`` in :ref:`tuning`.

``sig-signing-signatures``
   See the description of ``sig-signing-signatures`` in
   :ref:`tuning`.

``sig-signing-type``
   See the description of ``sig-signing-type`` in :ref:`tuning`.

``transfer-source``
   See the description of ``transfer-source`` in :ref:`zone_transfers`.

``transfer-source-v6``
   See the description of ``transfer-source-v6`` in :ref:`zone_transfers`.

``alt-transfer-source``
   See the description of ``alt-transfer-source`` in :ref:`zone_transfers`.

``alt-transfer-source-v6``
   See the description of ``alt-transfer-source-v6`` in :ref:`zone_transfers`.

``use-alt-transfer-source``
   See the description of ``use-alt-transfer-source`` in :ref:`zone_transfers`.

``notify-source``
   See the description of ``notify-source`` in :ref:`zone_transfers`.

``notify-source-v6``
   See the description of ``notify-source-v6`` in :ref:`zone_transfers`.

``min-refresh-time``; ``max-refresh-time``; ``min-retry-time``; ``max-retry-time``
   See the descriptions in :ref:`tuning`.

``ixfr-from-differences``
   See the description of ``ixfr-from-differences`` in :ref:`boolean_options`.
   (Note that the ``ixfr-from-differences`` choices of ``primary`` and ``secondary``
   are not available at the zone level.)

``key-directory``
   See the description of ``key-directory`` in :ref:`options`.

``auto-dnssec``
   See the description of ``auto-dnssec`` in :ref:`options`.

``serial-update-method``
   See the description of ``serial-update-method`` in :ref:`options`.

``inline-signing``
   If ``yes``, BIND 9 maintains a separate signed version of the zone.
   An unsigned zone is transferred in or loaded from disk and the signed
   version of the zone is served with, possibly, a different serial
   number. The signed version of the zone is stored in a file that is
   the zone's filename (set in ``file``) with a ``.signed`` extension.
   This behavior is disabled by default.

``multi-master``
   See the description of ``multi-master`` in :ref:`boolean_options`.

``masterfile-format``
   See the description of ``masterfile-format`` in :ref:`tuning`.

``max-zone-ttl``
   See the description of ``max-zone-ttl`` in :ref:`options`.

``dnssec-secure-to-insecure``
   See the description of ``dnssec-secure-to-insecure`` in :ref:`boolean_options`.

.. _dynamic_update_policies:

Dynamic Update Policies
^^^^^^^^^^^^^^^^^^^^^^^

BIND 9 supports two methods of granting clients the right to
perform dynamic updates to a zone:

- ``allow-update`` - a simple access control list
- ``update-policy`` - fine-grained access control

In both cases, BIND 9 writes the updates to the zone's filename
set in ``file``.

In the case of a DNSSEC zone, DNSSEC records are also written to
the zone's filename, unless ``inline-signing`` is enabled.

   .. note:: The zone file can no longer be manually updated while ``named``
      is running; it is now necessary to perform :option:`rndc freeze`, edit,
      and then perform :option:`rndc thaw`. Comments and formatting
      in the zone file are lost when dynamic updates occur.

The ``allow-update`` clause is a simple access control list. Any client
that matches the ACL is granted permission to update any record in the
zone.

The ``update-policy`` clause allows more fine-grained control over which
updates are allowed. It specifies a set of rules, in which each rule
either grants or denies permission for one or more names in the zone to
be updated by one or more identities. Identity is determined by the key
that signed the update request, using either TSIG or SIG(0). In most
cases, ``update-policy`` rules only apply to key-based identities. There
is no way to specify update permissions based on the client source address.

``update-policy`` rules are only meaningful for zones of type
``primary``, and are not allowed in any other zone type. It is a
configuration error to specify both ``allow-update`` and
``update-policy`` at the same time.

A pre-defined ``update-policy`` rule can be switched on with the command
``update-policy local;``. ``named`` automatically
generates a TSIG session key when starting and stores it in a file;
this key can then be used by local clients to update the zone while
``named`` is running. By default, the session key is stored in the file
``/var/run/named/session.key``, the key name is "local-ddns", and the
key algorithm is HMAC-SHA256. These values are configurable with the
``session-keyfile``, ``session-keyname``, and ``session-keyalg`` options,
respectively. A client running on the local system, if run with
appropriate permissions, may read the session key from the key file and
use it to sign update requests. The zone's update policy is set to
allow that key to change any record within the zone. Assuming the key
name is "local-ddns", this policy is equivalent to:

::

   update-policy { grant local-ddns zonesub any; };

with the additional restriction that only clients connecting from the
local system are permitted to send updates.

Note that only one session key is generated by ``named``; all zones
configured to use ``update-policy local`` accept the same key.

The command ``nsupdate -l`` implements this feature, sending requests to
localhost and signing them using the key retrieved from the session key
file.

Other rule definitions look like this:

::

   ( grant | deny ) identity ruletype  name   types

Each rule grants or denies privileges. Rules are checked in the order in
which they are specified in the ``update-policy`` statement. Once a
message has successfully matched a rule, the operation is immediately
granted or denied, and no further rules are examined. There are 13 types
of rules; the rule type is specified by the ``ruletype`` field, and the
interpretation of other fields varies depending on the rule type.

In general, a rule is matched when the key that signed an update request
matches the ``identity`` field, the name of the record to be updated
matches the ``name`` field (in the manner specified by the ``ruletype``
field), and the type of the record to be updated matches the ``types``
field. Details for each rule type are described below.

The ``identity`` field must be set to a fully qualified domain name. In
most cases, this represents the name of the TSIG or SIG(0) key that
must be used to sign the update request. If the specified name is a
wildcard, it is subject to DNS wildcard expansion, and the rule may
apply to multiple identities. When a TKEY exchange has been used to
create a shared secret, the identity of the key used to authenticate the
TKEY exchange is used as the identity of the shared secret. Some
rule types use identities matching the client's Kerberos principal (e.g,
``"host/machine@REALM"``) or Windows realm (``machine$@REALM``).

The ``name`` field also specifies a fully qualified domain name. This often
represents the name of the record to be updated. Interpretation of this
field is dependent on rule type.

If no ``types`` are explicitly specified, then a rule matches all types
except RRSIG, NS, SOA, NSEC, and NSEC3. Types may be specified by name,
including ``ANY``; ANY matches all types except NSEC and NSEC3, which can
never be updated. Note that when an attempt is made to delete all
records associated with a name, the rules are checked for each existing
record type.

The ruletype field has 16 values: ``name``, ``subdomain``, ``zonesub``, ``wildcard``,
``self``, ``selfsub``, ``selfwild``, ``ms-self``, ``ms-selfsub``, ``ms-subdomain``,
``krb5-self``, ``krb5-selfsub``, ``krb5-subdomain``,
``tcp-self``, ``6to4-self``, and ``external``.

``name``
    With exact-match semantics, this rule matches when the name being updated is identical to the contents of the ``name`` field.

``subdomain``
    This rule matches when the name being updated is a subdomain of, or identical to, the contents of the ``name`` field.

``zonesub``
    This rule is similar to subdomain, except that it matches when the name being updated is a subdomain of the zone in which the ``update-policy`` statement appears. This obviates the need to type the zone name twice, and enables the use of a standard ``update-policy`` statement in multiple zones without modification.
    When this rule is used, the ``name`` field is omitted.

``wildcard``
    The ``name`` field is subject to DNS wildcard expansion, and this rule matches when the name being updated is a valid expansion of the wildcard.

``self``
    This rule matches when the name of the record being updated matches the contents of the ``identity`` field. The ``name`` field is ignored. To avoid confusion, it is recommended that this field be set to the same value as the ``identity`` field or to "."
    The ``self`` rule type is most useful when allowing one key per name to update, where the key has the same name as the record to be updated. In this case, the ``identity`` field can be specified as ``*`` (asterisk).

``selfsub``
    This rule is similar to ``self``, except that subdomains of ``self`` can also be updated.

``selfwild``
    This rule is similar to ``self``, except that only subdomains of ``self`` can be updated.

``ms-self``
    When a client sends an UPDATE using a Windows machine principal (for example, ``machine$@REALM``), this rule allows records with the absolute name of ``machine.REALM`` to be updated.

    The realm to be matched is specified in the ``identity`` field.

    The ``name`` field has no effect on this rule; it should be set to "." as a placeholder.

    For example, ``grant EXAMPLE.COM ms-self . A AAAA`` allows any machine with a valid principal in the realm ``EXAMPLE.COM`` to update its own address records.

``ms-selfsub``
    This is similar to ``ms-self``, except it also allows updates to any subdomain of the name specified in the Windows machine principal, not just to the name itself.

``ms-subdomain``
    When a client sends an UPDATE using a Windows machine principal (for example, ``machine$@REALM``), this rule allows any machine in the specified realm to update any record in the zone or in a specified subdomain of the zone.

    The realm to be matched is specified in the ``identity`` field.

    The ``name`` field specifies the subdomain that may be updated. If set to "." or any other name at or above the zone apex, any name in the zone can be updated.

    For example, if ``update-policy`` for the zone "example.com" includes ``grant EXAMPLE.COM ms-subdomain hosts.example.com. AA AAAA``, any machine with a valid principal in the realm ``EXAMPLE.COM`` is able to update address records at or below ``hosts.example.com``.

``krb5-self``
    When a client sends an UPDATE using a Kerberos machine principal (for example, ``host/machine@REALM``), this rule allows records with the absolute name of ``machine`` to be updated, provided it has been authenticated by REALM. This is similar but not identical to ``ms-self``, due to the ``machine`` part of the Kerberos principal being an absolute name instead of an unqualified name.

    The realm to be matched is specified in the ``identity`` field.

    The ``name`` field has no effect on this rule; it should be set to "." as a placeholder.

    For example, ``grant EXAMPLE.COM krb5-self . A AAAA`` allows any machine with a valid principal in the realm ``EXAMPLE.COM`` to update its own address records.

``krb5-selfsub``
    This is similar to ``krb5-self``, except it also allows updates to any subdomain of the name specified in the ``machine`` part of the Kerberos principal, not just to the name itself.

``krb5-subdomain``
    This rule is identical to ``ms-subdomain``, except that it works with Kerberos machine principals (i.e., ``host/machine@REALM``) rather than Windows machine principals.

``tcp-self``
    This rule allows updates that have been sent via TCP and for which the standard mapping from the client's IP address into the ``in-addr.arpa`` and ``ip6.arpa`` namespaces matches the name to be updated. The ``identity`` field must match that name. The ``name`` field should be set to ".". Note that, since identity is based on the client's IP address, it is not necessary for update request messages to be signed.

    .. note::
        It is theoretically possible to spoof these TCP sessions.

``6to4-self``
    This allows the name matching a 6to4 IPv6 prefix, as specified in :rfc:`3056`, to be updated by any TCP connection from either the 6to4 network or from the corresponding IPv4 address. This is intended to allow NS or DNAME RRsets to be added to the ``ip6.arpa`` reverse tree.

    The ``identity`` field must match the 6to4 prefix in ``ip6.arpa``. The ``name`` field should be set to ".". Note that, since identity is based on the client's IP address, it is not necessary for update request messages to be signed.

    In addition, if specified for an ``ip6.arpa`` name outside of the ``2.0.0.2.ip6.arpa`` namespace, the corresponding /48 reverse name can be updated. For example, TCP/IPv6 connections from 2001:DB8:ED0C::/48 can update records at ``C.0.D.E.8.B.D.0.1.0.0.2.ip6.arpa``.

    .. note::
        It is theoretically possible to spoof these TCP sessions.

``external``
    This rule allows ``named`` to defer the decision of whether to allow a given update to an external daemon.

    The method of communicating with the daemon is specified in the ``identity`` field, the format of which is "``local:``\ path", where "path" is the location of a Unix-domain socket. (Currently, "local" is the only supported mechanism.)

    Requests to the external daemon are sent over the Unix-domain socket as datagrams with the following format:

    ::

        Protocol version number (4 bytes, network byte order, currently 1)
        Request length (4 bytes, network byte order)
        Signer (null-terminated string)
        Name (null-terminated string)
        TCP source address (null-terminated string)
        Rdata type (null-terminated string)
        Key (null-terminated string)
        TKEY token length (4 bytes, network byte order)
        TKEY token (remainder of packet)

    The daemon replies with a four-byte value in network byte order, containing either 0 or 1; 0 indicates that the specified update is not permitted, and 1 indicates that it is.

       .. warning:: The external daemon must not delay communication. This policy is evaluated synchronously; any wait period negatively affects :iscman:`named` performance.

.. _multiple_views:

Multiple Views
^^^^^^^^^^^^^^

When multiple views are in use, a zone may be referenced by more than
one of them. Often, the views contain different zones with the same
name, allowing different clients to receive different answers for the
same queries. At times, however, it is desirable for multiple views to
contain identical zones. The ``in-view`` zone option provides an
efficient way to do this; it allows a view to reference a zone that was
defined in a previously configured view. For example:

::

   view internal {
       match-clients { 10/8; };

       zone example.com {
       type primary;
       file "example-external.db";
       };
   };

   view external {
       match-clients { any; };

       zone example.com {
       in-view internal;
       };
   };

An ``in-view`` option cannot refer to a view that is configured later in
the configuration file.

A ``zone`` statement which uses the ``in-view`` option may not use any
other options, with the exception of ``forward`` and ``forwarders``.
(These options control the behavior of the containing view, rather than
change the zone object itself.)

Zone-level ACLs (e.g., allow-query, allow-transfer), and other
configuration details of the zone, are all set in the view the referenced
zone is defined in. Be careful to ensure that ACLs are wide
enough for all views referencing the zone.

An ``in-view`` zone cannot be used as a response policy zone.

An ``in-view`` zone is not intended to reference a ``forward`` zone.

.. _zone_file:

Zone File
---------

.. _types_of_resource_records_and_when_to_use_them:

Types of Resource Records and When to Use Them
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This section, largely borrowed from :rfc:`1034`, describes the concept of a
Resource Record (RR) and explains when each type is used. Since the
publication of :rfc:`1034`, several new RRs have been identified and
implemented in the DNS. These are also included.

Resource Records
^^^^^^^^^^^^^^^^

A domain name identifies a node. Each node has a set of resource
information, which may be empty. The set of resource information
associated with a particular name is composed of separate RRs. The order
of RRs in a set is not significant and need not be preserved by name
servers, resolvers, or other parts of the DNS. However, sorting of
multiple RRs is permitted for optimization purposes: for example, to
specify that a particular nearby server be tried first. See
:ref:`the_sortlist_statement` and :ref:`rrset_ordering`.

The components of a Resource Record are:

owner name
    The domain name where the RR is found.

type
    An encoded 16-bit value that specifies the type of the resource record.

TTL
    The time-to-live of the RR. This field is a 32-bit integer in units of seconds, and is primarily used by resolvers when they cache RRs. The TTL describes how long a RR can be cached before it should be discarded.

class
    An encoded 16-bit value that identifies a protocol family or an instance of a protocol.

RDATA
    The resource data. The format of the data is type- and sometimes class-specific.

For a complete list of *types* of valid RRs, including those that have been obsoleted, please refer to https://en.wikipedia.org/wiki/List_of_DNS_record_types.

The following *classes* of resource records are currently valid in the
DNS:

IN
    The Internet.

CH
    Chaosnet, a LAN protocol created at MIT in the mid-1970s. It was rarely used for its historical purpose, but was reused for BIND's built-in server information zones, e.g., ``version.bind``.

HS
    Hesiod, an information service developed by MIT's Project Athena. It was used to share information about various systems databases, such as users, groups, printers, etc.

The owner name is often implicit, rather than forming an integral part
of the RR. For example, many name servers internally form tree or hash
structures for the name space, and chain RRs off nodes. The remaining RR
parts are the fixed header (type, class, TTL), which is consistent for
all RRs, and a variable part (RDATA) that fits the needs of the resource
being described.

The TTL field is a time limit on how long an RR can be
kept in a cache. This limit does not apply to authoritative data in
zones; that also times out, but follows the refreshing policies for the
zone. The TTL is assigned by the administrator for the zone where the
data originates. While short TTLs can be used to minimize caching, and a
zero TTL prohibits caching, the realities of Internet performance
suggest that these times should be on the order of days for the typical
host. If a change is anticipated, the TTL can be reduced prior to
the change to minimize inconsistency, and then
increased back to its former value following the change.

The data in the RDATA section of RRs is carried as a combination of
binary strings and domain names. The domain names are frequently used as
"pointers" to other data in the DNS.

.. _rr_text:

Textual Expression of RRs
^^^^^^^^^^^^^^^^^^^^^^^^^

RRs are represented in binary form in the packets of the DNS protocol,
and are usually represented in highly encoded form when stored in a name
server or resolver. In the examples provided in :rfc:`1034`, a style
similar to that used in primary files was employed in order to show the
contents of RRs. In this format, most RRs are shown on a single line,
although continuation lines are possible using parentheses.

The start of the line gives the owner of the RR. If a line begins with a
blank, then the owner is assumed to be the same as that of the previous
RR. Blank lines are often included for readability.

Following the owner are listed the TTL, type, and class of the RR. Class
and type use the mnemonics defined above, and TTL is an integer before
the type field. To avoid ambiguity in parsing, type and class
mnemonics are disjoint, TTLs are integers, and the type mnemonic is
always last. The IN class and TTL values are often omitted from examples
in the interest of clarity.

The resource data or RDATA section of the RR is given using knowledge
of the typical representation for the data.

For example, the RRs carried in a message might be shown as:

 +---------------------+---------------+--------------------------------+
 | ``ISI.EDU.``        | ``MX``        | ``10 VENERA.ISI.EDU.``         |
 +---------------------+---------------+--------------------------------+
 |                     | ``MX``        | ``10 VAXA.ISI.EDU``            |
 +---------------------+---------------+--------------------------------+
 | ``VENERA.ISI.EDU``  | ``A``         | ``128.9.0.32``                 |
 +---------------------+---------------+--------------------------------+
 |                     | ``A``         | ``10.1.0.52``                  |
 +---------------------+---------------+--------------------------------+
 | ``VAXA.ISI.EDU``    | ``A``         | ``10.2.0.27``                  |
 +---------------------+---------------+--------------------------------+
 |                     | ``A``         | ``128.9.0.33``                 |
 +---------------------+---------------+--------------------------------+

The MX RRs have an RDATA section which consists of a 16-bit number
followed by a domain name. The address RRs use a standard IP address
format to contain a 32-bit Internet address.

The above example shows six RRs, with two RRs at each of three domain
names.

Here is another possible example:

 +----------------------+---------------+-------------------------------+
 | ``XX.LCS.MIT.EDU.``  | ``IN A``      | ``10.0.0.44``                 |
 +----------------------+---------------+-------------------------------+
 |                      | ``CH A``      | ``MIT.EDU. 2420``             |
 +----------------------+---------------+-------------------------------+

This shows two addresses for ``XX.LCS.MIT.EDU``, each of a
different class.

.. _mx_records:

Discussion of MX Records
~~~~~~~~~~~~~~~~~~~~~~~~

As described above, domain servers store information as a series of
resource records, each of which contains a particular piece of
information about a given domain name (which is usually, but not always,
a host). The simplest way to think of an RR is as a typed pair of data, a
domain name matched with a relevant datum and stored with some
additional type information, to help systems determine when the RR is
relevant.

MX records are used to control delivery of email. The data specified in
the record is a priority and a domain name. The priority controls the
order in which email delivery is attempted, with the lowest number
first. If two priorities are the same, a server is chosen randomly. If
no servers at a given priority are responding, the mail transport agent
falls back to the next largest priority. Priority numbers do not
have any absolute meaning; they are relevant only respective to other
MX records for that domain name. The domain name given is the machine to
which the mail is delivered. It *must* have an associated address
record (A or AAAA); CNAME is not sufficient.

For a given domain, if there is both a CNAME record and an MX record,
the MX record is in error and is ignored. Instead, the mail is
delivered to the server specified in the MX record pointed to by the
CNAME. For example:

 +------------------------+--------+--------+--------------+------------------------+
 | ``example.com.``       | ``IN`` | ``MX`` | ``10``       | ``mail.example.com.``  |
 +------------------------+--------+--------+--------------+------------------------+
 |                        | ``IN`` | ``MX`` | ``10``       | ``mail2.example.com.`` |
 +------------------------+--------+--------+--------------+------------------------+
 |                        | ``IN`` | ``MX`` | ``20``       | ``mail.backup.org.``   |
 +------------------------+--------+--------+--------------+------------------------+
 | ``mail.example.com.``  | ``IN`` | ``A``  | ``10.0.0.1`` |                        |
 +------------------------+--------+--------+--------------+------------------------+
 | ``mail2.example.com.`` | ``IN`` | ``A``  | ``10.0.0.2`` |                        |
 +------------------------+--------+--------+--------------+------------------------+

Mail delivery is attempted to ``mail.example.com`` and
``mail2.example.com`` (in any order); if neither of those succeeds,
delivery to ``mail.backup.org`` is attempted.

.. _Setting_TTLs:

Setting TTLs
~~~~~~~~~~~~

The time-to-live (TTL) of the RR field is a 32-bit integer represented in
units of seconds, and is primarily used by resolvers when they cache
RRs. The TTL describes how long an RR can be cached before it should be
discarded. The following three types of TTLs are currently used in a zone
file.

SOA
    The last field in the SOA is the negative caching TTL. This controls how long other servers cache no-such-domain (NXDOMAIN) responses from this server.

    The maximum time for negative caching is 3 hours (3h).

$TTL
    The $TTL directive at the top of the zone file (before the SOA) gives a default TTL for every RR without a specific TTL set.

RR TTLs
    Each RR can have a TTL as the second field in the RR, which controls how long other servers can cache it.

All of these TTLs default to units of seconds, though units can be
explicitly specified: for example, ``1h30m``.

.. _ipv4_reverse:

Inverse Mapping in IPv4
~~~~~~~~~~~~~~~~~~~~~~~

Reverse name resolution (that is, translation from IP address to name)
is achieved by means of the ``in-addr.arpa`` domain and PTR records.
Entries in the in-addr.arpa domain are made in least-to-most significant
order, read left to right. This is the opposite order to the way IP
addresses are usually written. Thus, a machine with an IP address of
10.1.2.3 would have a corresponding in-addr.arpa name of
3.2.1.10.in-addr.arpa. This name should have a PTR resource record whose
data field is the name of the machine or, optionally, multiple PTR
records if the machine has more than one name. For example, in the
``example.com`` domain:

 +--------------+-------------------------------------------------------+
 | ``$ORIGIN``  | ``2.1.10.in-addr.arpa``                               |
 +--------------+-------------------------------------------------------+
 | ``3``        | ``IN PTR foo.example.com.``                           |
 +--------------+-------------------------------------------------------+

.. note::

   The ``$ORIGIN`` line in this example is only to provide context;
   it does not necessarily appear in the actual
   usage. It is only used here to indicate that the example is
   relative to the listed origin.

.. _zone_directives:

Other Zone File Directives
~~~~~~~~~~~~~~~~~~~~~~~~~~

The DNS "master file" format was initially defined in :rfc:`1035` and has
subsequently been extended. While the format itself is class-independent,
all records in a zone file must be of the same class.

Master file directives include ``$ORIGIN``, ``$INCLUDE``, and ``$TTL.``

.. _atsign:

The ``@`` (at-sign)
^^^^^^^^^^^^^^^^^^^

When used in the label (or name) field, the asperand or at-sign (@)
symbol represents the current origin. At the start of the zone file, it
is the <``zone_name``>, followed by a trailing dot (.).

.. _origin_directive:

The ``$ORIGIN`` Directive
^^^^^^^^^^^^^^^^^^^^^^^^^

Syntax: ``$ORIGIN`` domain-name [comment]

``$ORIGIN`` sets the domain name that is appended to any
unqualified records. When a zone is first read, there is an implicit
``$ORIGIN`` <``zone_name``>``.``; note the trailing dot. The
current ``$ORIGIN`` is appended to the domain specified in the
``$ORIGIN`` argument if it is not absolute.

::

   $ORIGIN example.com.
   WWW     CNAME   MAIN-SERVER

is equivalent to

::

   WWW.EXAMPLE.COM. CNAME MAIN-SERVER.EXAMPLE.COM.

.. _include_directive:

The ``$INCLUDE`` Directive
^^^^^^^^^^^^^^^^^^^^^^^^^^

Syntax: ``$INCLUDE`` filename [origin] [comment]

This reads and processes the file ``filename`` as if it were included in the
file at this point. The ``filename`` can be an absolute path, or a relative
path. In the latter case it is read from ``named``'s working directory. If
``origin`` is specified, the file is processed with ``$ORIGIN`` set to that
value; otherwise, the current ``$ORIGIN`` is used.

The origin and the current domain name revert to the values they had
prior to the ``$INCLUDE`` once the file has been read.

.. note::

   :rfc:`1035` specifies that the current origin should be restored after
   an ``$INCLUDE``, but it is silent on whether the current domain name
   should also be restored. BIND 9 restores both of them. This could be
   construed as a deviation from :rfc:`1035`, a feature, or both.

.. _ttl_directive:

The ``$TTL`` Directive
^^^^^^^^^^^^^^^^^^^^^^

Syntax: ``$TTL`` default-ttl [comment]

This sets the default Time-To-Live (TTL) for subsequent records with undefined
TTLs. Valid TTLs are of the range 0-2147483647 seconds.

``$TTL`` is defined in :rfc:`2308`.

.. _generate_directive:

BIND Primary File Extension: the ``$GENERATE`` Directive
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Syntax: ``$GENERATE`` range owner [ttl] [class] type rdata [comment]

``$GENERATE`` is used to create a series of resource records that only
differ from each other by an iterator.

``range``
    This can be one of two forms: start-stop or start-stop/step.
    If the first form is used, then step is set to 1. "start",
    "stop", and "step" must be positive integers between 0 and
    (2^31)-1. "start" must not be larger than "stop".

``owner``
    This describes the owner name of the resource records to be created.

    The ``owner`` string may include one or more ``$`` (dollar sign)
    symbols, which will be replaced with the iterator value when
    generating records; see below for details.

``ttl``
    This specifies the time-to-live of the generated records. If
    not specified, this is inherited using the normal TTL inheritance
    rules.

    ``class`` and ``ttl`` can be entered in either order.

``class``
    This specifies the class of the generated records. This must
    match the zone class if it is specified.

    ``class`` and ``ttl`` can be entered in either order.

``type``
    This can be any valid type.

``rdata``
    This is a string containing the RDATA of the resource record
    to be created. As with ``owner``, the ``rdata`` string may
    include one or more ``$`` symbols, which are replaced with the
    iterator value. ``rdata`` may be quoted if there are spaces in
    the string; the quotation marks do not appear in the generated
    record.

    Any single ``$`` (dollar sign) symbols within the ``owner`` or
    ``rdata`` strings are replaced by the iterator value. To get a ``$``
    in the output, escape the ``$`` using a backslash ``\\``, e.g.,
    ``\$``. (For compatibility with earlier versions, ``$$`` is also
    recognized as indicating a literal ``$`` in the output.)

    The ``$`` may optionally be followed by modifiers which change
    the offset from the iterator, field width, and base.  Modifiers
    are introduced by a ``{`` (left brace) immediately following
    the ``$``, as in  ``${offset[,width[,base]]}``. For example,
    ``${-20,3,d}`` subtracts 20 from the current value and prints
    the result as a decimal in a zero-padded field of width 3.
    Available output forms are decimal (``d``), octal (``o``),
    hexadecimal (``x`` or ``X`` for uppercase), and nibble (``n``
    or ``N`` for uppercase). The modfiier cannot contain whitespace
    or newlines.

    The default modifier is ``${0,0,d}``. If the ``owner`` is not
    absolute, the current ``$ORIGIN`` is appended to the name.

    In nibble mode, the value is treated as if it were a reversed
    hexadecimal string, with each hexadecimal digit as a separate
    label. The width field includes the label separator.

Examples:

``$GENERATE`` can be used to easily generate the sets of records required
to support sub-/24 reverse delegations described in :rfc:`2317`:

::

   $ORIGIN 0.0.192.IN-ADDR.ARPA.
   $GENERATE 1-2 @ NS SERVER$.EXAMPLE.
   $GENERATE 1-127 $ CNAME $.0

is equivalent to

::

   0.0.0.192.IN-ADDR.ARPA. NS SERVER1.EXAMPLE.
   0.0.0.192.IN-ADDR.ARPA. NS SERVER2.EXAMPLE.
   1.0.0.192.IN-ADDR.ARPA. CNAME 1.0.0.0.192.IN-ADDR.ARPA.
   2.0.0.192.IN-ADDR.ARPA. CNAME 2.0.0.0.192.IN-ADDR.ARPA.
   ...
   127.0.0.192.IN-ADDR.ARPA. CNAME 127.0.0.0.192.IN-ADDR.ARPA.

This example creates a set of A and MX records. Note the MX's ``rdata``
is a quoted string; the quotes are stripped when ``$GENERATE`` is processed:

::

   $ORIGIN EXAMPLE.
   $GENERATE 1-127 HOST-$ A 1.2.3.$
   $GENERATE 1-127 HOST-$ MX "0 ."

is equivalent to

::

   HOST-1.EXAMPLE.   A  1.2.3.1
   HOST-1.EXAMPLE.   MX 0 .
   HOST-2.EXAMPLE.   A  1.2.3.2
   HOST-2.EXAMPLE.   MX 0 .
   HOST-3.EXAMPLE.   A  1.2.3.3
   HOST-3.EXAMPLE.   MX 0 .
   ...
   HOST-127.EXAMPLE. A  1.2.3.127
   HOST-127.EXAMPLE. MX 0 .


This example generates A and AAAA records using modifiers; the AAAA
``owner`` names are generated using nibble mode:

::

   $ORIGIN EXAMPLE.
   $GENERATE 0-2 HOST-${0,4,d} A 1.2.3.${1,0,d}
   $GENERATE 1024-1026 ${0,3,n} AAAA 2001:db8::${0,4,x}

is equivalent to:

::
   HOST-0000.EXAMPLE.   A      1.2.3.1
   HOST-0001.EXAMPLE.   A      1.2.3.2
   HOST-0002.EXAMPLE.   A      1.2.3.3
   0.0.4.EXAMPLE.       AAAA   2001:db8::400
   1.0.4.EXAMPLE.       AAAA   2001:db8::401
   2.0.4.EXAMPLE.       AAAA   2001:db8::402

The ``$GENERATE`` directive is a BIND extension and not part of the
standard zone file format.

.. _zonefile_format:

Additional File Formats
~~~~~~~~~~~~~~~~~~~~~~~

In addition to the standard text format, BIND 9 supports the ability
to read or dump to zone files in other formats.

The ``raw`` format is a binary representation of zone data in a manner
similar to that used in zone transfers. Since it does not require
parsing text, load time is significantly reduced.

An even faster alternative is the ``map`` format, which is an image of a
BIND 9 in-memory zone database; it can be loaded directly into memory via
the ``mmap()`` function and the zone can begin serving queries almost
immediately.  Because records are not indivdually processed when loading a
``map`` file, zones using this format cannot be used in ``response-policy``
statements.

For a primary server, a zone file in ``raw`` or ``map`` format is expected
to be generated from a text zone file by the ``named-compilezone`` command.
For a secondary server or a dynamic zone, the zone file is automatically
generated when ``named`` dumps the zone contents after zone transfer or
when applying prior updates, if one of these formats is specified by the
``masterfile-format`` option.

If a zone file in a binary format needs manual modification, it first must
be converted to ``text`` format by the ``named-compilezone`` command,
then converted back after editing.  For example:

::
    named-compilezone -f map -F text -o zonefile.text <origin> zonefile.map
    [edit zonefile.text]
    named-compilezone -f text -F map -o zonefile.map <origin> zonefile.text

Note that the ``map`` format is highly architecture-specific. A ``map``
file *cannot* be used on a system with different pointer size, endianness,
or data alignment than the system on which it was generated, and should in
general be used only inside a single system.

The ``map`` format is also dependent on the internal memory representation
of a zone database, which may change from one release of BIND 9 to another.
``map`` files are never compatible across major releases, and may not be
compatible across minor releases; any upgrade to BIND 9 may cause ``map``
files to be rejected when loading. If a ``map`` file is being used for a
primary zone, it will need to be regenerated from text before restarting
the server.  If it used for a secondary zone, this is unnecessary; the
rejection of the file will trigger a retransfer of the zone from the
primary. (To avoid a spike in traffic upon restart, it may be desirable in
some cases to convert ``map`` files to ``text`` format using
``named-compilezone`` before an upgrade, then back to ``map`` format with
the new version of ``named-compilezone`` afterward.)

The use of ``map`` format may also be limited by operating system
mmap(2) limits like ``sysctl vm.max_map_count``.  For Linux, this
defaults to 65536, which limits the number of mapped zones that can
be used without increasing ``vm.max_map_count``.

``raw`` format uses network byte order and avoids architecture-
dependent data alignment so that it is as portable as possible, but it is
still primarily expected to be used inside the same single system. To
export a zone file in either ``raw`` or ``map`` format, or make a portable
backup of such a file, conversion to ``text`` format is recommended.

.. _statistics:

BIND 9 Statistics
-----------------

BIND 9 maintains lots of statistics information and provides several
interfaces for users to access those statistics. The available
statistics include all statistics counters that are meaningful in BIND 9,
and other information that is considered useful.

The statistics information is categorized into the following sections:

Incoming Requests
   The number of incoming DNS requests for each OPCODE.

Incoming Queries
   The number of incoming queries for each RR type.

Outgoing Queries
   The number of outgoing queries for each RR type sent from the internal
   resolver, maintained per view.

Name Server Statistics
   Statistics counters for incoming request processing.

Zone Maintenance Statistics
   Statistics counters regarding zone maintenance operations, such as zone
   transfers.

Resolver Statistics
   Statistics counters for name resolutions performed in the internal resolver,
   maintained per view.

Cache DB RRsets
   Statistics counters related to cache contents, maintained per view.

   The "NXDOMAIN" counter is the number of names that have been cached as
   nonexistent.  Counters named for RR types indicate the number of active
   RRsets for each type in the cache database.

   If an RR type name is preceded by an exclamation point (!), it represents the
   number of records in the cache which indicate that the type does not exist
   for a particular name; this is also known as "NXRRSET". If an RR type name
   is preceded by a hash mark (#), it represents the number of RRsets for this
   type that are present in the cache but whose TTLs have expired; these RRsets
   may only be used if stale answers are enabled.  If an RR type name is
   preceded by a tilde (~), it represents the number of RRsets for this type
   that are present in the cache database but are marked for garbage collection;
   these RRsets cannot be used.

Socket I/O Statistics
   Statistics counters for network-related events.

A subset of Name Server Statistics is collected and shown per zone for
which the server has the authority, when ``zone-statistics`` is set to
``full`` (or ``yes``), for backward compatibility. See the description of
``zone-statistics`` in :ref:`options` for further details.

These statistics counters are shown with their zone and view names. The
view name is omitted when the server is not configured with explicit
views.

There are currently two user interfaces to get access to the statistics.
One is in plain-text format, dumped to the file specified by the
``statistics-file`` configuration option; the other is remotely
accessible via a statistics channel when the ``statistics-channels``
statement is specified in the configuration file (see :ref:`statschannels`.)

.. _statsfile:

The Statistics File
~~~~~~~~~~~~~~~~~~~

The text format statistics dump begins with a line, like:

``+++ Statistics Dump +++ (973798949)``

The number in parentheses is a standard Unix-style timestamp, measured
in seconds since January 1, 1970. Following that line is a set of
statistics information, which is categorized as described above. Each
section begins with a line, like:

``++ Name Server Statistics ++``

Each section consists of lines, each containing the statistics counter
value followed by its textual description; see below for available
counters. For brevity, counters that have a value of 0 are not shown in
the statistics file.

The statistics dump ends with the line where the number is identical to
the number in the beginning line; for example:

``--- Statistics Dump --- (973798949)``

.. _statistics_counters:

Statistics Counters
~~~~~~~~~~~~~~~~~~~

The following lists summarize the statistics counters that BIND 9 provides.
For each counter, the abbreviated
symbol name is given; these symbols are shown in the statistics
information accessed via an HTTP statistics channel.
The description of the counter is also shown in the
statistics file but, in this document, may be slightly
modified for better readability.

.. _stats_counters:

Name Server Statistics Counters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``Requestv4``
    This indicates the number of IPv4 requests received. Note: this also counts non-query requests.

``Requestv6``
    This indicates the number of IPv6 requests received. Note: this also counts non-query requests.

``ReqEdns0``
    This indicates the number of requests received with EDNS(0).

``ReqBadEDN SVer``
    This indicates the number of requests received with an unsupported EDNS version.

``ReqTSIG``
    This indicates the number of requests received with TSIG.

``ReqSIG0``
    This indicates the number of requests received with SIG(0).

``ReqBadSIG``
    This indicates the number of requests received with an invalid (TSIG or SIG(0)) signature.

``ReqTCP``
    This indicates the number of TCP requests received.

``AuthQryRej``
    This indicates the number of rejected authoritative (non-recursive) queries.

``RecQryRej``
    This indicates the number of rejected recursive queries.

``XfrRej``
    This indicates the number of rejected zone transfer requests.

``UpdateRej``
    This indicates the number of rejected dynamic update requests.

``Response``
    This indicates the number of responses sent.

``RespTruncated``
    This indicates the number of truncated responses sent.

``RespEDNS0``
    This indicates the number of responses sent with EDNS(0).

``RespTSIG``
    This indicates the number of responses sent with TSIG.

``RespSIG0``
    This indicates the number of responses sent with SIG(0).

``QrySuccess``
    This indicates the number of queries that resulted in a successful answer, meaning queries which return a NOERROR response with at least one answer RR. This corresponds to the ``success`` counter of previous versions of BIND 9.

``QryAuthAns``
    This indicates the number of queries that resulted in an authoritative answer.

``QryNoauthAns``
    This indicates the number of queries that resulted in a non-authoritative answer.

``QryReferral``
    This indicates the number of queries that resulted in a referral answer. This corresponds to the ``referral`` counter of previous versions of BIND 9.

``QryNxrrset``
    This indicates the number of queries that resulted in NOERROR responses with no data. This corresponds to the ``nxrrset`` counter of previous versions of BIND 9.

``QrySERVFAIL``
    This indicates the number of queries that resulted in SERVFAIL.

``QryFORMERR``
    This indicates the number of queries that resulted in FORMERR.

``QryNXDOMAIN``
    This indicates the number of queries that resulted in NXDOMAIN. This corresponds to the ``nxdomain`` counter of previous versions of BIND 9.

``QryRecursion``
    This indicates the number of queries that caused the server to perform recursion in order to find the final answer. This corresponds to the ``recursion`` counter of previous versions of BIND 9.

``QryDuplicate``
    This indicates the number of queries which the server attempted to recurse but for which it discovered an existing query with the same IP address, port, query ID, name, type, and class already being processed. This corresponds to the ``duplicate`` counter of previous versions of BIND 9.

``QryDropped``
    This indicates the number of recursive queries for which the server discovered an excessive number of existing recursive queries for the same name, type, and class, and which were subsequently dropped. This is the number of dropped queries due to the reason explained with the ``clients-per-query`` and ``max-clients-per-query`` options (see :ref:`clients-per-query <clients-per-query>`). This corresponds to the ``dropped`` counter of previous versions of BIND 9.

``QryFailure``
    This indicates the number of query failures. This corresponds to the ``failure`` counter of previous versions of BIND 9. Note: this counter is provided mainly for backward compatibility with previous versions; normally, more fine-grained counters such as ``AuthQryRej`` and ``RecQryRej`` that would also fall into this counter are provided, so this counter is not of much interest in practice.

``QryNXRedir``
    This indicates the number of queries that resulted in NXDOMAIN that were redirected.

``QryNXRedirRLookup``
    This indicates the number of queries that resulted in NXDOMAIN that were redirected and resulted in a successful remote lookup.

``XfrReqDone``
    This indicates the number of requested and completed zone transfers.

``UpdateReqFwd``
    This indicates the number of forwarded update requests.

``UpdateRespFwd``
    This indicates the number of forwarded update responses.

``UpdateFwdFail``
    This indicates the number of forwarded dynamic updates that failed.

``UpdateDone``
    This indicates the number of completed dynamic updates.

``UpdateFail``
    This indicates the number of failed dynamic updates.

``UpdateBadPrereq``
    This indicates the number of dynamic updates rejected due to a prerequisite failure.

``UpdateQuota``
    This indicates the number of times a dynamic update or update
    forwarding request was rejected because the number of pending
    requests exceeded ``update-quota``.

``RateDropped``
    This indicates the number of responses dropped due to rate limits.

``RateSlipped``
    This indicates the number of responses truncated by rate limits.

``RPZRewrites``
    This indicates the number of response policy zone rewrites.

.. _zone_stats:

Zone Maintenance Statistics Counters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``NotifyOutv4``
    This indicates the number of IPv4 notifies sent.

``NotifyOutv6``
    This indicates the number of IPv6 notifies sent.

``NotifyInv4``
    This indicates the number of IPv4 notifies received.

``NotifyInv6``
    This indicates the number of IPv6 notifies received.

``NotifyRej``
    This indicates the number of incoming notifies rejected.

``SOAOutv4``
    This indicates the number of IPv4 SOA queries sent.

``SOAOutv6``
    This indicates the number of IPv6 SOA queries sent.

``AXFRReqv4``
    This indicates the number of requested IPv4 AXFRs.

``AXFRReqv6``
    This indicates the number of requested IPv6 AXFRs.

``IXFRReqv4``
    This indicates the number of requested IPv4 IXFRs.

``IXFRReqv6``
    This indicates the number of requested IPv6 IXFRs.

``XfrSuccess``
    This indicates the number of successful zone transfer requests.

``XfrFail``
    This indicates the number of failed zone transfer requests.

.. _resolver_stats:

Resolver Statistics Counters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``Queryv4``
    This indicates the number of IPv4 queries sent.

``Queryv6``
    This indicates the number of IPv6 queries sent.

``Responsev4``
    This indicates the number of IPv4 responses received.

``Responsev6``
    This indicates the number of IPv6 responses received.

``NXDOMAIN``
    This indicates the number of NXDOMAINs received.

``SERVFAIL``
    This indicates the number of SERVFAILs received.

``FORMERR``
    This indicates the number of FORMERRs received.

``OtherError``
    This indicates the number of other errors received.

``EDNS0Fail``
    This indicates the number of EDNS(0) query failures.

``Mismatch``
    This indicates the number of mismatched responses received, meaning the DNS ID, response's source address, and/or the response's source port does not match what was expected. (The port must be 53 or as defined by the ``port`` option.) This may be an indication of a cache poisoning attempt.

``Truncated``
    This indicates the number of truncated responses received.

``Lame``
    This indicates the number of lame delegations received.

``Retry``
    This indicates the number of query retries performed.

``QueryAbort``
    This indicates the number of queries aborted due to quota control.

``QuerySockFail``
    This indicates the number of failures in opening query sockets. One common reason for such failures is due to a limitation on file descriptors.

``QueryTimeout``
    This indicates the number of query timeouts.

``GlueFetchv4``
    This indicates the number of IPv4 NS address fetches invoked.

``GlueFetchv6``
    This indicates the number of IPv6 NS address fetches invoked.

``GlueFetchv4Fail``
    This indicates the number of failed IPv4 NS address fetches.

``GlueFetchv6Fail``
    This indicates the number of failed IPv6 NS address fetches.

``ValAttempt``
    This indicates the number of attempted DNSSEC validations.

``ValOk``
    This indicates the number of successful DNSSEC validations.

``ValNegOk``
    This indicates the number of successful DNSSEC validations on negative information.

``ValFail``
    This indicates the number of failed DNSSEC validations.

``QryRTTnn``
    This provides a frequency table on query round-trip times (RTTs). Each ``nn`` specifies the corresponding frequency. In the sequence of ``nn_1``, ``nn_2``, ..., ``nn_m``, the value of ``nn_i`` is the number of queries whose RTTs are between ``nn_(i-1)`` (inclusive) and ``nn_i`` (exclusive) milliseconds. For the sake of convenience, we define ``nn_0`` to be 0. The last entry should be represented as ``nn_m+``, which means the number of queries whose RTTs are equal to or greater than ``nn_m`` milliseconds.

.. _socket_stats:

Socket I/O Statistics Counters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Socket I/O statistics counters are defined per socket type, which are
``UDP4`` (UDP/IPv4), ``UDP6`` (UDP/IPv6), ``TCP4`` (TCP/IPv4), ``TCP6``
(TCP/IPv6), ``Unix`` (Unix Domain), and ``FDwatch`` (sockets opened
outside the socket module). In the following list, ``<TYPE>`` represents
a socket type. Not all counters are available for all socket types;
exceptions are noted in the descriptions.

``<TYPE>Open``
    This indicates the number of sockets opened successfully. This counter does not apply to the ``FDwatch`` type.

``<TYPE>OpenFail``
    This indicates the number of failures to open sockets. This counter does not apply to the ``FDwatch`` type.

``<TYPE>Close``
    This indicates the number of closed sockets.

``<TYPE>BindFail``
    This indicates the number of failures to bind sockets.

``<TYPE>ConnFail``
    This indicates the number of failures to connect sockets.

``<TYPE>Conn``
    This indicates the number of connections established successfully.

``<TYPE>AcceptFail``
    This indicates the number of failures to accept incoming connection requests. This counter does not apply to the ``UDP`` and ``FDwatch`` types.

``<TYPE>Accept``
    This indicates the number of incoming connections successfully accepted. This counter does not apply to the ``UDP`` and ``FDwatch`` types.

``<TYPE>SendErr``
    This indicates the number of errors in socket send operations.

``<TYPE>RecvErr``
    This indicates the number of errors in socket receive operations, including errors of send operations on a connected UDP socket, notified by an ICMP error message.
