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

.. _reference:

Configuration Reference
=======================

The operational functionality of BIND 9 is defined using the file
**named.conf**, which is typically located in /etc or /usr/local/etc/namedb,
depending on the operating system or distribution. A further file **rndc.conf**
will be present if **rndc** is being run from a remote host, but is not required
if rndc is being run from **localhost** (the same system as BIND 9 is running
on).

.. _named_conf:

Configuration File (named.conf)
-------------------------------

The file :file:`named.conf` may contain three types of entities:

.. glossary::

   Comment
      :ref:`Multiple comment formats are supported <comment_syntax>`.

   Block
      :ref:`Blocks <configuration_blocks>` are containers for :term:`statements
      <Statement>` which either have common functionality - for example,
      the definition of a cryptographic key in a :namedconf:ref:`key` block - or which
      define the scope of the statement - for example, a statement which appears
      in a :namedconf:ref:`zone` block has scope only for that zone.

      Blocks are organized hierarchically within :file:`named.conf` and may have a
      number of different properties:

      - Certain blocks cannot be nested inside other blocks and thus may be
        regarded as the *topmost-level* blocks: for example, the
        :namedconf:ref:`options` block and the :namedconf:ref:`logging` block.

      - Certain blocks can appear multiple times, in which case they have
        an associated name to disambiguate them: for example, the
        :namedconf:ref:`zone` block (``zone example.com { ... };``) or the
        :namedconf:ref:`key` block (``key mykey { ... };``).

      - Certain blocks may be "nested" within other blocks. For example, the
        :namedconf:ref:`zone` block may appear within a
        :namedconf:ref:`view` block.

      The description of each block in this manual lists its permissible locations.

   Statement
      - Statements define and control specific BIND behaviors.
      - Statements may have a single parameter (a **Value**) or multiple parameters
        (**Argument/Value** pairs). For example, the :any:`recursion` statement takes a
        single value parameter - in this case, the string ``yes`` or ``no``
        (``recursion yes;``) - while the :namedconf:ref:`port` statement takes a  numeric value
        defining the DNS port number (``port 53;``). More complex statements take one or
        more argument/value pairs. The :any:`also-notify` statement may take a number
        of such argument/value pairs, such as ``also-notify port 5353;``,
        where ``port`` is the argument and ``5353`` is the corresponding value.
      - Statements can appear in a single :term:`block <Block>` - for
        example, an :namedconf:ref:`algorithm` statement can appear only in a
        :namedconf:ref:`key` block - or in multiple blocks - for example, an
        :any:`also-notify` statement can appear in an :namedconf:ref:`options`
        block where it has global (server-wide) scope, in a :any:`zone`
        block where it has scope only for the specific zone (and overrides
        any global statement), or even in a :any:`view` block where it has
        scope for only that view (and overrides any global statement).

The file :file:`named.conf` may further contain one or more instances of the
:ref:`include <include_grammar>` **Directive**. This directive is provided for
administrative convenience in assembling a complete :file:`named.conf` file and plays
no subsequent role in BIND 9 operational characteristics or functionality.

.. Note::
   Over a period of many years the BIND ARM acquired a bewildering array of
   terminology. Many of the terms used described similar concepts and served
   only to add a layer of complexity, possibly confusion, and perhaps mystique
   to BIND 9 configuration. The ARM now uses only the terms **Block**,
   **Statement**, **Argument**, **Value**, and **Directive** to describe all
   entities used in BIND 9 configuration.

.. _comment_syntax:

Comment Syntax
~~~~~~~~~~~~~~

The BIND 9 comment syntax allows comments to appear anywhere that
whitespace may appear in a BIND configuration file. To appeal to
programmers of all kinds, they can be written in the C, C++, or
shell/Perl style.

Syntax
^^^^^^

.. code-block:: none

   /* This is a BIND comment as in C */

.. code-block:: none

   // This is a BIND comment as in C++

.. code-block:: none

   # This is a BIND comment as in common Unix shells
   # and Perl

Definition and Usage
^^^^^^^^^^^^^^^^^^^^

Comments can be inserted anywhere that whitespace may appear in a BIND
configuration file.

C-style comments start with the two characters /\* (slash, star) and end
with \*/ (star, slash). Because they are completely delimited with these
characters, they can be used to comment only a portion of a line or to
span multiple lines.

C-style comments cannot be nested. For example, the following is not
valid because the entire comment ends with the first \*/:

.. code-block:: none

   /* This is the start of a comment.
      This is still part of the comment.
   /* This is an incorrect attempt at nesting a comment. */
      This is no longer in any comment. */

C++-style comments start with the two characters // (slash, slash) and
continue to the end of the physical line. They cannot be continued
across multiple physical lines; to have one logical comment span
multiple lines, each line must use the // pair. For example:

.. code-block:: none

   // This is the start of a comment.  The next line
   // is a new comment, even though it is logically
   // part of the previous comment.

Shell-style (or Perl-style) comments start with the
character ``#`` (number/pound sign) and continue to the end of the physical
line, as in C++ comments. For example:

.. code-block:: none

   # This is the start of a comment.  The next line
   # is a new comment, even though it is logically
   # part of the previous comment.

.. warning::

   The semicolon (``;``) character cannot start a comment, unlike
   in a zone file. The semicolon indicates the end of a
   configuration statement.

Configuration Layout Styles
~~~~~~~~~~~~~~~~~~~~~~~~~~~

BIND is very picky about opening and closing brackets/braces, semicolons, and
all the other separators defined in the formal syntaxes in later sections.
There are many layout styles that can assist in minimizing errors, as shown in
the following examples:

.. code-block:: none

	// dense single-line style
	zone "example.com" in{type secondary; file "secondary.example.com"; primaries {10.0.0.1;};};
	//  single-statement-per-line style
	zone "example.com" in{
		type secondary;
		file "secondary.example.com";
		primaries {10.0.0.1;};
	};
	// spot the difference
	zone "example.com" in{
		type secondary;
	file "sec.secondary.com";
	primaries {10.0.0.1;}; };

.. _include_grammar:

``include`` Directive
~~~~~~~~~~~~~~~~~~~~~

.. code-block:: none

   include filename;

.. _include_statement:

``include`` Directive Definition and Usage
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The include directive inserts the specified file (or files if a valid `glob
expression`_ is detected) at the point where the include directive is
encountered. The include directive facilitates the administration of
configuration files by permitting the reading or writing of some things but not
others. For example, the statement could include private keys that are readable
only by the name server.

.. _`glob expression`: https://man7.org/linux/man-pages/man7/glob.7.html

.. _address_match_lists:

Address Match Lists
~~~~~~~~~~~~~~~~~~~

Syntax
^^^^^^

An address match list is a list of semicolon-separated :term:`address_match_element` s.

::

   { <address_match_element>; ... };

Each element is then defined as:

.. glossary::

   ``address_match_element``

    ::

        [ ! ] ( <ip_address> | <netprefix> | key <server_key> | <acl_name> | { address_match_list } )

Definition and Usage
^^^^^^^^^^^^^^^^^^^^

Address match lists are primarily used to determine access control for
various server operations. They are also used in the :any:`listen-on` and
:any:`sortlist` statements. The elements which constitute an address match
list can be any of the following:

- :term:`ip_address`: an IP address (IPv4 or IPv6)

- :term:`netprefix`: an IP prefix (in ``/`` notation)

- :term:`server_key`: a key ID, as defined by the ``key`` statement

- :term:`acl_name`: the name of an address match list defined with the :any:`acl` statement

-  a nested address match list enclosed in braces

Elements can be negated with a leading exclamation mark (``!``), and the
match list names "any", "none", "localhost", and "localnets" are
predefined. More information on those names can be found in the
description of the :any:`acl` statement.

The addition of the key clause made the name of this syntactic element
something of a misnomer, since security keys can be used to validate
access without regard to a host or network address. Nonetheless, the
term "address match list" is still used throughout the documentation.

When a given IP address or prefix is compared to an address match list,
the comparison takes place in approximately O(1) time. However, key
comparisons require that the list of keys be traversed until a matching
key is found, and therefore may be somewhat slower.

The interpretation of a match depends on whether the list is being used
for access control, defining :any:`listen-on` ports, or in a :any:`sortlist`,
and whether the element was negated.

When used as an access control list, a non-negated match allows access
and a negated match denies access. If there is no match, access is
denied. The clauses :any:`allow-notify`, :any:`allow-recursion`,
:any:`allow-recursion-on`, :any:`allow-query`, :any:`allow-query-on`,
:any:`allow-query-cache`, :any:`allow-query-cache-on`, :any:`allow-transfer`,
:any:`allow-update`, :any:`allow-update-forwarding`, and :any:`blackhole`
all use address match lists. Similarly, the
:any:`listen-on` option causes the server to refuse queries on any of
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


Glossary of Terms Used
~~~~~~~~~~~~~~~~~~~~~~

Following is a list of terms used throughout the BIND configuration
file documentation:

.. glossary::

    ``acl_name``
        The name of an :term:`address_match_list` as defined by the :any:`acl` statement.

    ``address_match_list``
        See :ref:`address_match_lists`.

    ``boolean``
        Either ``yes`` or ``no``. The words ``true`` and ``false`` are also accepted, as are the numbers ``1`` and ``0``.

    ``domain_name``
        A quoted string which is used as a DNS name; for example: ``my.test.domain``.

    ``duration``
        A duration in BIND 9 can be written in three ways: as a single number
        representing seconds, as a string of numbers with TTL-style
        time-unit suffixes, or in ISO 6801 duration format.

        Allowed TTL time-unit suffixes are: "W" (week), "D" (day), "H" (hour),
        "M" (minute), and "S" (second). Examples: "1W" (1 week), "3d12h"
        (3 days, 12 hours).

        ISO 8601 duration format consists of the letter "P", followed by an
	optional series of numbers with unit suffixes "Y" (year), "M" (month),
        "W" (week), and "D" (day); this may optionally be followed by the
        letter "T", and another series of numbers with unit suffixes
        "H" (hour), "M" (minute), and "S" (second). Examples: "P3M10D"
        (3 months, 10 days), "P2WT12H" (2 weeks, 12 hours), "pt15m"
        (15 minutes).  For more information on ISO 8601 duration format,
        see :rfc:`3339`, appendix A.

        Both TTL-style and ISO 8601 duration formats are case-insensitive.

    ``fixedpoint``
        A non-negative real number that can be specified to the nearest one-hundredth. Up to five digits can be specified before a decimal point, and up to two digits after, so the maximum value is 99999.99. Acceptable values might be further limited by the contexts in which they are used.

    ``integer``
        A non-negative 32-bit integer (i.e., a number between 0 and 4294967295, inclusive). Its acceptable value might be further limited by the context in which it is used.

    ``ip_address``
        An :term:`ipv4_address` or :term:`ipv6_address`.

    ``ipv4_address``
        An IPv4 address with exactly four integer elements valued 0 through 255
        and separated by dots (``.``), such as ``192.168.1.1`` (a
        "dotted-decimal" notation with all four elements present).

    ``ipv6_address``
        An IPv6 address, such as ``2001:db8::1234``. IPv6-scoped addresses that have ambiguity on their scope zones must be disambiguated by an appropriate zone ID with the percent character (``%``) as a delimiter. It is strongly recommended to use string zone names rather than numeric identifiers, to be robust against system configuration changes. However, since there is no standard mapping for such names and identifier values, only interface names as link identifiers are supported, assuming one-to-one mapping between interfaces and links. For example, a link-local address ``fe80::1`` on the link attached to the interface ``ne0`` can be specified as ``fe80::1%ne0``. Note that on most systems link-local addresses always have ambiguity and need to be disambiguated.

    ``netprefix``
        An IP network specified as an :term:`ip_address`, followed by a slash (``/``) and then the number of bits in the netmask. Trailing zeros in an :term:`ip_address` may be omitted. For example, ``127/8`` is the network ``127.0.0.0`` with netmask ``255.0.0.0`` and ``1.2.3.0/28`` is network ``1.2.3.0`` with netmask ``255.255.255.240``.
        When specifying a prefix involving an IPv6-scoped address, the scope may be omitted. In that case, the prefix matches packets from any scope.

    ``percentage``
         An integer value followed by ``%`` to represent percent.

    ``port``
        An IP port :term:`integer`. It is limited to 0 through 65535, with values below 1024 typically restricted to use by processes running as root. In some cases, an asterisk (``*``) character can be used as a placeholder to select a random high-numbered port.

    ``portrange``
        A list of a :term:`port` or a port range. A port range is specified in the form of ``range`` followed by two :term:`port` s, ``port_low`` and ``port_high``, which represents port numbers from ``port_low`` through ``port_high``, inclusive. ``port_low`` must not be larger than ``port_high``. For example, ``range 1024 65535`` represents ports from 1024 through 65535. The asterisk (``*``) character is not allowed as a valid :term:`port` or as a port range boundary.

    ``server-list``
        A named list of one or more :term:`ip_address` es with optional :term:`tls_id`, :term:`server_key`, and/or :term:`port`. A ``server-list`` list may include other ``server-list`` lists.

    ``server_key``
        A :term:`domain_name` representing the name of a shared key, to be used for
        :ref:`transaction security <tsig>`. Keys are defined using
        :namedconf:ref:`key` blocks.

    ``size``
    ``sizeval``
        A 64-bit unsigned integer. Integers may take values 0 <= value <= 18446744073709551615, though certain parameters (such as :any:`max-journal-size`) may use a more limited range within these extremes. In most cases, setting a value to 0 does not literally mean zero; it means "undefined" or "as big as possible," depending on the context. See the explanations of particular parameters that use ``size`` for details on how they interpret its use. Numeric values can optionally be followed by a scaling factor: ``K`` or ``k`` for kilobytes, ``M`` or ``m`` for megabytes, and ``G`` or ``g`` for gigabytes, which scale by 1024, 1024*1024, and 1024*1024*1024 respectively.

        Some statements also accept the keywords ``unlimited`` or ``default``:
        ``unlimited`` generally means "as big as possible," and is usually the best way to safely set a very large number.
        ``default`` uses the limit that was in force when the server was started.

    ``tls_id``
        A named TLS configuration object which defines a TLS key and certificate. See :any:`tls` block.


.. _configuration_file_grammar:

.. _configuration_blocks:

Blocks
------

A BIND 9 configuration consists of blocks, statements, and comments.

The following blocks are supported:

    :any:`acl`
        Defines a named IP address matching list, for access control and other uses.

    :any:`controls`
        Declares control channels to be used by the :iscman:`rndc` utility.

    :any:`dnssec-policy`
        Describes a DNSSEC key and signing policy for zones. See :any:`dnssec-policy` for details.

    :namedconf:ref:`key`
        Specifies key information for use in authentication and authorization using TSIG.

    :any:`key-store`
        Describes a DNSSEC key store. See :ref:`key-store Grammar <key_store_grammar>` for details.

    :any:`logging`
        Specifies what information the server logs and where the log messages are sent.

    :namedconf:ref:`options`
        Controls global server configuration options and sets defaults for other statements.

    :namedconf:ref:`remote-servers`
        Defines a named list of servers for inclusion in various zone statements such as :any:`parental-agents`, :any:`primaries` or :any:`also-notify` lists.

    :namedconf:ref:`server`
        Sets certain configuration options on a per-server basis.

    :any:`statistics-channels`
        Declares communication channels to get access to :iscman:`named` statistics.

    :any:`tls`
        Specifies configuration information for a TLS connection, including a :any:`key-file`, :any:`cert-file`, :any:`ca-file`, :any:`dhparam-file`, :any:`remote-hostname`, :any:`ciphers`, :any:`protocols`, :any:`prefer-server-ciphers`, and :any:`session-tickets`.

    :any:`http`
        Specifies configuration information for an HTTP connection, including :any:`endpoints`, :any:`listener-clients`, and :any:`streams-per-connection`.

    :any:`trust-anchors`
        Defines DNSSEC trust anchors: if used with the ``initial-key`` or ``initial-ds`` keyword, trust anchors are kept up-to-date using :rfc:`5011` trust anchor maintenance; if used with ``static-key`` or ``static-ds``, keys are permanent.

    :any:`managed-keys`
        Is identical to :any:`trust-anchors`; this option is deprecated in favor of :any:`trust-anchors` with the ``initial-key`` keyword, and may be removed in a future release.

    :any:`trusted-keys`
        Defines permanent trusted DNSSEC keys; this option is deprecated in favor of :any:`trust-anchors` with the ``static-key`` keyword, and may be removed in a future release.

    :any:`view`
        Defines a view.

    :any:`zone`
        Defines a zone.

The :any:`logging` and :namedconf:ref:`options` statements may only occur once per
configuration.

:any:`acl` Block Grammar
~~~~~~~~~~~~~~~~~~~~~~~~~

.. namedconf:statement:: acl
   :tags: server
   :short: Assigns a symbolic name to an address match list.

:any:`acl` Block Definition and Usage
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The :any:`acl` statement assigns a symbolic name to an address match list.
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

:any:`controls` Block Grammar
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
.. namedconf:statement:: controls
   :tags: server
   :short: Specifies control channels to be used to manage the name server.

:any:`controls` Block Definition and Usage
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The :any:`controls` statement declares control channels to be used by
system administrators to manage the operation of the name server. These
control channels are used by the :iscman:`rndc` utility to send commands to
and retrieve non-DNS results from a name server.

.. namedconf:statement:: unix
   :tags: obsolete
   :short: Specifies a Unix domain socket as a control channel.

   This option has been removed and using it will cause a fatal error.

.. namedconf:statement:: inet
   :tags: server
   :short: Specifies a TCP socket as a control channel.

   An :any:`inet` control channel is a TCP socket listening at the specified
   :term:`port` on the specified :term:`ip_address`, which can be an IPv4 or IPv6
   address. An :term:`ip_address` of ``*`` (asterisk) is interpreted as the IPv4
   wildcard address; connections are accepted on any of the system's
   IPv4 addresses. To listen on the IPv6 wildcard address, use an
   :term:`ip_address` of ``::``. If :iscman:`rndc` is only used on the local host,
   using the loopback address (``127.0.0.1`` or ``::1``) is recommended for
   maximum security.

   If no port is specified, port 953 is used. The asterisk ``*`` cannot
   be used for :term:`port`.

   The ability to issue commands over the control channel is restricted by
   the ``allow`` and :any:`keys` clauses.

   ``allow``
      Connections to the control channel
      are permitted based on the :term:`address_match_list`. This is for simple IP
      address-based filtering only; any :term:`server_key` elements of the
      :term:`address_match_list` are ignored.

   :any:`keys`
      The primary authorization mechanism of the command channel is the
      list of :term:`server_key` s. Each listed
      :namedconf:ref:`key` is authorized to execute commands over the control
      channel. See :ref:`admin_tools` for information about
      configuring keys in :iscman:`rndc`.


``read-only``
   If the ``read-only`` argument is ``on``, the control channel is limited
   to the following set of read-only commands: ``nta -dump``, :any:`null`,
   ``status``, ``showzone``, ``testgen``, and ``zonestatus``. By default,
   ``read-only`` is not enabled and the control channel allows read-write
   access.

If no :any:`controls` statement is present, :iscman:`named` sets up a default
control channel listening on the loopback address 127.0.0.1 and its IPv6
counterpart, ::1. In this case, and also when the :any:`controls` statement
is present but does not have a :any:`keys` clause, :iscman:`named` attempts
to load the command channel key from the file |rndc_key|.
To create an ``rndc.key`` file, run :option:`rndc-confgen -a`.

To disable the command channel, use an empty :any:`controls` statement:
``controls { };``.


``key`` Block Grammar
~~~~~~~~~~~~~~~~~~~~~~~~~
.. namedconf:statement:: key
   :tags: security
   :short: Defines a shared secret key for use with :ref:`tsig` or the command channel.

``key`` Block Definition and Usage
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ``key`` statement defines a shared secret key for use with TSIG (see
:ref:`tsig`) or the command channel (see :any:`controls`).

The ``key`` statement can occur at the top level of the configuration
file or inside a :any:`view` statement. Keys defined in top-level ``key``
statements can be used in all views. Keys intended for use in a
:any:`controls` statement must be defined at the top level.

The :term:`server_key`, also known as the key name, is a domain name that uniquely
identifies the key. It can be used in a :namedconf:ref:`server` statement to cause
requests sent to that server to be signed with this key, or in address
match lists to verify that incoming requests have been signed with a key
matching this name, algorithm, and secret.

.. namedconf:statement:: algorithm
   :tags: security
   :short: Defines the algorithm to be used in a key clause.

   The ``algorithm_id`` is a string that specifies a security/authentication
   algorithm. The :iscman:`named` server supports ``hmac-md5``, ``hmac-sha1``,
   ``hmac-sha224``, ``hmac-sha256``, ``hmac-sha384``, and ``hmac-sha512``
   TSIG authentication. Truncated hashes are supported by appending the
   minimum number of required bits preceded by a dash, e.g.,
   ``hmac-sha1-80``.

.. namedconf:statement:: secret
   :tags: security
   :short: Defines a Base64-encoded string to be used as the secret by the algorithm.

   The ``secret_string`` is the secret to be used by the
   algorithm, and is treated as a Base64-encoded string.

.. _key_store_grammar:

:any:`key-store` Block Grammar
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
.. namedconf:statement:: key-store
   :tags: dnssec
   :short: Configures a DNSSEC key store.

.. _key_store_statement:

``key-store`` Block Definition and Usage
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ``key-store`` statement defines how DNSSEC keys should be stored.

There is one built-in key store named ``key-directory``. Configuring
keys to use ``key-store "key-directory"`` is identical to using
``key-directory``.

The following options can be specified in a :any:`key-store` statement:

.. directory

   The ``directory`` specifies where key files for this key should be stored.
   This is similar to using the zone's ``key-directory``.

.. namedconf:statement:: pkcs11-uri
   :tags: dnssec, pkcs11

   The ``uri`` is a string that specifies a PKCS#11 URI Scheme (defined in
   :rfc:`7512`). When set, :iscman:`named` tries to create keys inside the
   corresponding PKCS#11 token. This requires BIND to be built with OpenSSL 3,
   and to have a PKCS#11 provider configured.

.. _logging_grammar:

:any:`logging` Block Grammar
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
.. namedconf:statement:: logging
   :tags: logging
   :short: Configures logging options for the name server.

:any:`logging` Block Definition and Usage
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The :any:`logging` statement configures a wide variety of logging options
for the name server. Its :any:`channel` phrase associates output methods,
format options, and severity levels with a name that can then be used
with the :any:`category` phrase to select how various classes of messages
are logged.

Only one :any:`logging` statement is used to define as many channels and
categories as desired. If there is no :any:`logging` statement, the
logging configuration is:

::

   logging {
        category default { default_syslog; default_debug; };
        category unmatched { null; };
   };

If :iscman:`named` is started with the :option:`-L <named -L>` option, it logs to the specified
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
the default channels, or to standard error if the :option:`-g <named -g>` option was
specified.

The :any:`channel` Phrase
^^^^^^^^^^^^^^^^^^^^^^^^^
.. namedconf:statement:: channel
   :tags: logging
   :short: Defines a stream of data that can be independently logged.

All log output goes to one or more ``channels``; there is no limit to
the number of channels that can be created.

Every channel definition must include a destination clause that says
whether messages selected for the channel go to a file, go to a particular
syslog facility, go to the standard error stream, or are discarded. The definition can
optionally also limit the message severity level that is accepted
by the channel (the default is ``info``), and whether to include a
:iscman:`named`-generated time stamp, the category name, and/or the severity level
(the default is not to include any).

.. namedconf:statement:: null
   :tags: logging
   :short: Causes all messages sent to the logging channel to be discarded.

   The :any:`null` destination clause causes all messages sent to the channel
   to be discarded; in that case, other options for the channel are
   meaningless.

``file``
   The ``file`` destination clause directs the channel to a disk file. It
   can include additional arguments to specify how large the file is
   allowed to become before it is rolled to a backup file (``size``), how
   many backup versions of the file are saved each time this happens
   (``versions``), and the format to use for naming backup versions
   (``suffix``).

   The ``size`` option is used to limit log file growth. If the file ever
   exceeds the specified size, then :iscman:`named` stops writing to the file
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

.. namedconf:statement:: syslog
   :tags: logging
   :short: Directs the logging channel to the system log.

   The :any:`syslog` destination clause directs the channel to the system log.
   Its argument is a syslog facility as described in the :any:`syslog` man
   page. Known facilities are ``kern``, ``user``, ``mail``, ``daemon``,
   ``auth``, :any:`syslog`, ``lpr``, ``news``, ``uucp``, ``cron``,
   ``authpriv``, ``ftp``, ``local0``, ``local1``, ``local2``, ``local3``,
   ``local4``, ``local5``, ``local6``, and ``local7``; however, not all
   facilities are supported on all operating systems. How :any:`syslog`
   handles messages sent to this facility is described in the
   ``syslog.conf`` man page. On a system which uses a very old
   version of :any:`syslog`, which only uses two arguments to the ``openlog()``
   function, this clause is silently ignored.

.. namedconf:statement:: severity
   :tags: logging
   :short: Defines the priority level of log messages.

   The :any:`severity` clause works like :any:`syslog`'s "priorities," except
   that they can also be used when writing straight to a file rather
   than using :any:`syslog`. Messages which are not at least of the severity
   level given are not selected for the channel; messages of higher
   severity levels are accepted.

   When using :any:`syslog`, the ``syslog.conf`` priorities
   also determine what eventually passes through. For example, defining a
   channel facility and severity as ``daemon`` and ``debug``, but only
   logging ``daemon.warning`` via ``syslog.conf``, causes messages of
   severity ``info`` and ``notice`` to be dropped. If the situation were
   reversed, with :iscman:`named` writing messages of only ``warning`` or higher,
   then ``syslogd`` would print all messages it received from the channel.

.. namedconf:statement:: stderr
   :tags: logging
   :short: Directs the logging channel output to the server's standard error stream.

   The :any:`stderr` destination clause directs the channel to the server's
   standard error stream. This is intended for use when the server is
   running as a foreground process, as when debugging a
   configuration, for example.

The server can supply extensive debugging information when it is in
debugging mode. If the server's global debug level is greater than zero,
debugging mode is active. The global debug level is set either
by starting the :iscman:`named` server with the :option:`-d <named -d>` flag followed by a
positive integer, or by running :option:`rndc trace`. The global debug level
can be set to zero, and debugging mode turned off, by running ``rndc
notrace``. All debugging messages in the server have a debug level;
higher debug levels give more detailed output. Channels that indicate a specific debug severity
get debugging output of level 3 or less any time the server is in
debugging mode, regardless of the global debugging level:

::

   channel specific_debug_level {
       file "foo";
       severity debug 3;
   };

Channels with
``dynamic`` severity use the server's global debug level to determine
which messages to print.

.. namedconf:statement:: print-time
   :tags: logging
   :short: Specifies the time format for log messages.

   :any:`print-time` can be set to ``yes``, ``no``, or a time format
   specifier, which may be one of ``local``, ``iso8601``, or
   ``iso8601-utc``. If set to ``no``, the date and time are not
   logged. If set to ``yes`` or ``local``, the date and time are logged in
   a human-readable format, using the local time zone. If set to
   ``iso8601``, the local time is logged in ISO 8601 format. If set to
   ``iso8601-utc``, the date and time are logged in ISO 8601 format,
   with time zone set to UTC. The default is ``no``.

   :any:`print-time` may be specified for a :any:`syslog` channel, but it is
   usually pointless since :any:`syslog` also logs the date and time.

.. namedconf:statement:: print-category
   :tags: logging
   :short: Includes the category in log messages.

   If :any:`print-category` is requested, then the category of the message
   is logged as well.

.. namedconf:statement:: print-severity
   :tags: logging
   :short: Includes the severity in log messages.

   If :any:`print-severity` is on, then the
   severity level of the message is logged. The ``print-`` options may
   be used in any combination, and are always printed in the following
   order: time, category, severity.

Here is an example where all three ``print-`` options are on:

``28-Feb-2000 15:05:32.863 general: notice: running``

.. namedconf:statement:: buffered
   :tags: logging
   :short: Controls flushing of log messages.

   If :any:`buffered` has been turned on, the output to files is not
   flushed after each log entry. By default all log messages are flushed.

There are four predefined channels that are used for :iscman:`named`'s default
logging, as follows. If :iscman:`named` is started with the :option:`-L <named -L>` option, then a fifth
channel, ``default_logfile``, is added. How they are used is described in
:any:`category`.

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

For security reasons, when the :option:`-u <named -u>` command-line option is used, the
``named.run`` file is created only after :iscman:`named` has changed to the
new UID, and any debug output generated while :iscman:`named` is starting -
and still running as root - is discarded. To capture this
output, run the server with the :option:`-L <named -L>` option to specify a
default logfile, or the :option:`-g <named -g>` option to log to standard error which can
be redirected to a file.

Once a channel is defined, it cannot be redefined. The
built-in channels cannot be altered directly, but the default logging
can be modified by pointing categories at defined channels.

The :any:`category` Phrase
^^^^^^^^^^^^^^^^^^^^^^^^^^
There are many categories, so desired logs can be sent anywhere
while unwanted logs are ignored. If
a list of channels is not specified for a category, log messages in that
category are sent to the ``default`` category instead. If no
default category is specified, the following "default default" is used:

::

   category default { default_syslog; default_debug; };

If :iscman:`named` is started with the :option:`-L <named -L>` option, the default category
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


To discard all messages in a category, specify the :namedconf:ref:`null` channel:

::

   category xfer-out { null; };
   category notify { null; };

.. namedconf:statement:: category
   :tags: logging
   :short: Specifies the type of data logged to a particular channel.

The following are the available categories and brief descriptions of the
types of log information they contain. More categories may be added in
future BIND releases.

.. include:: logging-categories.inc.rst

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

``remote-servers`` Block Grammar
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
.. namedconf:statement:: remote-servers
   :tags: server
   :short: Defines a list of servers to be used by primary and secondary zones.

This specifies a list that allows for a common set of servers to be easily used
by multiple zones. The following options may reference to a list of
remote servers: :any:`parental-agents`, :any:`primaries`, and :any:`also-notify`.

A "parental agent" is a trusted DNS server that is queried to check whether DS
records for a given zones are up-to-date.

A "primary server" is where a secondary server can request zone transfers from.

To force the zone transfer requests to be sent over TLS, use :any:`tls` keyword,
e.g. ``primaries { 192.0.2.1 tls tls-configuration-name; };``,
where ``tls-configuration-name`` refers to a previously defined
:any:`tls statement <tls>`.

.. warning::

   Please note that TLS connections to primaries are **not
   authenticated** unless :any:`remote-hostname` or :any:`ca-file` are specified
   within the :any:`tls statement <tls>` in use (see information on
   :ref:`Strict TLS <strict-tls>` and :ref:`Mutual TLS <mutual-tls>`
   for more details).  **Not authenticated mode** (:ref:`Opportunistic
   TLS <opportunistic-tls>`) provides protection from passive
   observers but does not protect from man-in-the-middle attacks on
   zone transfers.

``options`` Block Grammar
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
.. namedconf:statement:: options
   :tags: server
   :short: Defines global options to be used by BIND 9.

This is the grammar of the ``options`` statement in the :iscman:`named.conf`
file:

``options`` Block Definition and Usage
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ``options`` statement sets up global options to be used by BIND.
This statement may appear only once in a configuration file. If there is
no ``options`` statement, an options block with each option set to its
default is used.

.. namedconf:statement:: attach-cache
   :tags: view
   :short: Allows multiple views to share a single cache database.

   This option allows multiple views to share a single cache database. Each view has
   its own cache database by default, but if multiple views have the
   same operational policy for name resolution and caching, those views
   can share a single cache to save memory, and possibly improve
   resolution efficiency, by using this option.

   The :any:`attach-cache` option may also be specified in :any:`view`
   statements, in which case it overrides the global :any:`attach-cache`
   option.

   The ``cache_name`` specifies the cache to be shared. When the :iscman:`named`
   server configures views which are supposed to share a cache, it
   creates a cache with the specified name for the first view of these
   sharing views. The rest of the views simply refer to the
   already-created cache.

   One common configuration to share a cache is to allow all views
   to share a single cache. This can be done by specifying
   :any:`attach-cache` as a global option with an arbitrary name.

   Another possible operation is to allow a subset of all views to share
   a cache while the others retain their own caches. For example, if
   there are three views A, B, and C, and only A and B should share a
   cache, specify the :any:`attach-cache` option as a view of A (or B)'s
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
   views: :any:`check-names`, :any:`dnssec-accept-expired`,
   :any:`dnssec-validation`, :any:`max-cache-ttl`, :any:`max-ncache-ttl`,
   :any:`max-stale-ttl`, :any:`max-cache-size`, :any:`min-cache-ttl`,
   :any:`min-ncache-ttl`, and :any:`zero-no-soa-ttl`.

   Note that there may be other parameters that may cause confusion if
   they are inconsistent for different views that share a single cache.
   For example, if these views define different sets of forwarders that
   can return different answers for the same question, sharing the
   answer does not make sense or could even be harmful. It is the
   administrator's responsibility to ensure that configuration differences in
   different views do not cause disruption with a shared cache.

.. namedconf:statement:: directory
   :tags: server
   :short: Sets the server's working directory.

   This sets the working directory of the server. Any non-absolute pathnames in
   the configuration file are taken as relative to this directory.
   The default location for most server output files (e.g.,
   ``named.run``) is this directory. If a directory is not specified,
   the working directory defaults to ``"."``, the directory from
   which the server was started. The directory specified should be an
   absolute path, and *must* be writable by the effective user ID of the
   :iscman:`named` process.

   The option takes effect only at the time that the configuration
   option is parsed; if other files are being included before or after specifying the
   new :any:`directory`, the :any:`directory` option must be listed
   before any other directive (like ``include``) that can work with relative
   files. The safest way to include files is to use absolute file names.

.. namedconf:statement:: dnstap
   :tags: logging
   :short: Enables logging of :any:`dnstap` messages.

   :any:`dnstap` is a fast, flexible method for capturing and logging DNS
   traffic. Developed by Robert Edmonds at Farsight Security, Inc., and
   supported by multiple DNS implementations, :any:`dnstap` uses
   ``libfstrm`` (a lightweight high-speed framing library; see
   https://github.com/farsightsec/fstrm) to send event payloads which
   are encoded using Protocol Buffers (``libprotobuf-c``, a mechanism
   for serializing structured data developed by Google, Inc.; see
   https://protobuf.dev).

   To enable :any:`dnstap` at compile time, the ``fstrm`` and
   ``protobuf-c`` libraries must be available, and BIND must be
   configured with ``--enable-dnstap``.

   The :any:`dnstap` option is a bracketed list of message types to be
   logged. These may be set differently for each view. Supported types
   are ``client``, ``auth``, ``resolver``, ``forwarder``, and
   ``update``. Specifying type ``all`` causes all :any:`dnstap`
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

   .. note:: In the default configuration, the dnstap output for
      recursive resolver traffic does not include the IP addresses used
      by server-side sockets. This is caused by the fact that unless the
      :ref:`query source address <query_address>` is explicitly set,
      these sockets are bound to wildcard IP addresses and determining
      the specific IP address used by each of them requires issuing a
      system call (i.e. incurring a performance penalty).

   Logged :any:`dnstap` messages can be parsed using the :iscman:`dnstap-read`
   utility (see :ref:`man_dnstap-read` for details).

   For more information on :any:`dnstap`, see https://dnstap.info.

   The ``fstrm`` library has a number of tunables that are exposed in
   :iscman:`named.conf`, and can be modified if necessary to improve
   performance or prevent loss of data. These are:

   .. namedconf:statement:: fstrm-set-buffer-hint
      :tags: logging
      :short: Sets the number of accumulated bytes in the output buffer before forcing a buffer flush.

      The indicates the threshold number of bytes to
      accumulate in the output buffer before forcing a buffer flush. The
      minimum is 1024, the maximum is 65536, and the default is 8192.

   .. namedconf:statement:: fstrm-set-flush-timeout
      :tags: logging
      :short: Sets the number of seconds that unflushed data remains in the output buffer.

      This is the number of seconds to allow
      unflushed data to remain in the output buffer. The minimum is 1
      second, the maximum is 600 seconds (10 minutes), and the default
      is 1 second.

   .. namedconf:statement:: fstrm-set-output-notify-threshold
      :tags: logging
      :short: Sets the number of outstanding queue entries allowed on an input queue before waking the I/O thread.

      This indicates the number of outstanding
      queue entries to allow on an input queue before waking the I/O
      thread. The minimum is 1 and the default is 32.

   .. namedconf:statement:: fstrm-set-output-queue-model
      :tags: logging
      :short: Sets the queuing semantics to use for queue objects.

      This sets the queuing semantics
      to use for queue objects. The default is ``mpsc`` (multiple
      producer, single consumer); the other option is ``spsc`` (single
      producer, single consumer).

   .. namedconf:statement:: fstrm-set-input-queue-size
      :tags: logging
      :short: Sets the number of queue entries to allocate for each input queue.

      This is the number of queue entries to
      allocate for each input queue. This value must be a power of 2.
      The minimum is 2, the maximum is 16384, and the default is 512.

   .. namedconf:statement:: fstrm-set-output-queue-size
      :tags: logging
      :short: Sets the number of queue entries allocated for each output queue.

      This specifies the number of queue entries to
      allocate for each output queue. The minimum is 2, the maximum is
      system-dependent and based on ``IOV_MAX``, and the default is 64.

   .. namedconf:statement:: fstrm-set-reopen-interval
      :tags: logging
      :short: Sets the number of seconds to wait between attempts to reopen a closed output stream.

      This sets the number of seconds to wait
      between attempts to reopen a closed output stream. The minimum is
      1 second, the maximum is 600 seconds (10 minutes), and the default
      is 5 seconds. For convenience, TTL-style time-unit suffixes may be
      used to specify the value.

   Note that all of the above minimum, maximum, and default values are
   set by the ``libfstrm`` library, and may be subject to change in
   future versions of the library. See the ``libfstrm`` documentation
   for more information.

.. namedconf:statement:: dnstap-output
   :tags: logging
   :short: Configures the path to which the :any:`dnstap` frame stream is sent.

   This configures the path to which the :any:`dnstap` frame stream is sent
   if :any:`dnstap` is enabled at compile time and active.

   The first argument is either ``file`` or ``unix``, indicating whether
   the destination is a file or a Unix domain socket. The second
   argument is the path of the file or socket. (Note: when using a
   socket, :any:`dnstap` messages are only sent if another process such
   as ``fstrm_capture`` (provided with ``libfstrm``) is listening on the
   socket.)

   If the first argument is ``file``, then up to three additional
   options can be added: ``size`` indicates the size to which a
   :any:`dnstap` log file can grow before being rolled to a new file;
   ``versions`` specifies the number of rolled log files to retain; and
   ``suffix`` indicates whether to retain rolled log files with an
   incrementing counter as the suffix (``increment``) or with the
   current timestamp (``timestamp``). These are similar to the ``size``,
   ``versions``, and ``suffix`` options in a :any:`logging` channel. The
   default is to allow :any:`dnstap` log files to grow to any size without
   rolling.

   :any:`dnstap-output` can only be set globally in :namedconf:ref:`options`. Currently,
   it can only be set once while :iscman:`named` is running; once set, it
   cannot be changed by :option:`rndc reload` or :option:`rndc reconfig`.

.. namedconf:statement:: dnstap-identity
   :tags: logging
   :short: Specifies an ``identity`` string to send in :any:`dnstap` messages.

   This specifies an ``identity`` string to send in :any:`dnstap` messages. If
   set to :any:`hostname`, which is the default, the server's hostname
   is sent. If set to ``none``, no identity string is sent.

.. namedconf:statement:: dnstap-version
   :tags: logging
   :short: Specifies a :any:`version` string to send in :any:`dnstap` messages.

   This specifies a :any:`version` string to send in :any:`dnstap` messages. The
   default is the version number of the BIND release. If set to
   ``none``, no version string is sent.

.. namedconf:statement:: geoip-directory
   :tags: server
   :short: Specifies the directory containing GeoIP database files.

   When :iscman:`named` is compiled using the MaxMind GeoIP2 geolocation API, this
   specifies the directory containing GeoIP database files.  By default, the
   option is set based on the prefix used to build the ``libmaxminddb`` module;
   for example, if the library is installed in ``/usr/local/lib``, then the
   default :any:`geoip-directory` is ``/usr/local/share/GeoIP``. See :any:`acl`
   for details about ``geoip`` ACLs.

.. namedconf:statement:: key-directory
   :tags: dnssec
   :short: Indicates the directory where public and private DNSSEC key files are found.

   This is the directory where the public and private DNSSEC key files should be
   found when performing a dynamic update of secure zones, if different
   than the current working directory. (Note that this option has no
   effect on the paths for files containing non-DNSSEC keys, such as
   ``rndc.key``, or ``session.key``.)

.. namedconf:statement:: lmdb-mapsize
   :tags: server
   :short: Sets a maximum size for the memory map of the new-zone database in LMDB database format.

   When :iscman:`named` is built with liblmdb, this option sets a maximum size
   for the memory map of the new-zone database (NZD) in LMDB database
   format. This database is used to store configuration information for
   zones added using :option:`rndc addzone`. Note that this is not the NZD
   database file size, but the largest size that the database may grow
   to.

   Because the database file is memory-mapped, its size is limited by
   the address space of the :iscman:`named` process. The default of 32 megabytes
   was chosen to be usable with 32-bit :iscman:`named` builds. The largest
   permitted value is 1 terabyte. Given typical zone configurations
   without elaborate ACLs, a 32 MB NZD file ought to be able to hold
   configurations of about 100,000 zones.

.. namedconf:statement:: managed-keys-directory
   :tags: dnssec
   :short: Specifies the directory in which to store the files that track managed DNSSEC keys.

   This specifies the directory in which to store the files that track managed DNSSEC
   keys (i.e., those configured using the ``initial-key`` or ``initial-ds``
   keywords in a :any:`trust-anchors` statement). By default, this is the working
   directory. The directory *must* be writable by the effective user ID of the
   :iscman:`named` process.

   If :iscman:`named` is not configured to use views, managed keys for
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

.. namedconf:statement:: max-ixfr-ratio
   :tags: transfer
   :short: Sets the maximum size for IXFR responses to zone transfer requests.

   This sets the size threshold (expressed as a percentage of the size
   of the full zone) beyond which :iscman:`named` chooses to use an AXFR
   response rather than IXFR when answering zone transfer requests. See
   :ref:`incremental_zone_transfers`.

   The minimum value is ``1%``. The keyword ``unlimited`` disables ratio
   checking and allows IXFRs of any size. The default is ``100%``.

.. namedconf:statement:: new-zones-directory
   :tags: zone
   :short: Specifies the directory where configuration parameters are stored for zones added by :option:`rndc addzone`.

   This specifies the directory in which to store the configuration
   parameters for zones added via :option:`rndc addzone`. By default, this is
   the working directory. If set to a relative path, it is relative
   to the working directory. The directory *must* be writable by the
   effective user ID of the :iscman:`named` process.

.. namedconf:statement:: qname-minimization
   :tags: query
   :short: Controls QNAME minimization behavior in the BIND 9 resolver.

   When this is set to ``strict``, BIND follows the QNAME minimization
   algorithm to the letter, as specified in :rfc:`7816`.

   Setting this option to ``relaxed`` causes BIND to fall back to
   normal (non-minimized) query mode when it receives either NXDOMAIN
   or other unexpected responses (e.g., SERVFAIL, improper zone
   cut, REFUSED) to a minimized query.

   In ``relaxed`` mode :iscman:`named` makes NS queries for ``<domain>`` as it
   walks down the tree.

   ``disabled`` disables QNAME minimization completely.
   ``off`` is a synonym for ``disabled``.

   The current default is ``relaxed``, but it may be changed to
   ``strict`` in a future release.

.. namedconf:statement:: tkey-gssapi-keytab
   :tags: security
   :short: Sets the KRB5 keytab file to use for GSS-TSIG updates.

   This is the KRB5 keytab file to use for GSS-TSIG updates. If this option is
   set and ``tkey-gssapi-credential`` is not set, updates are
   allowed with any key matching a principal in the specified keytab.

.. namedconf:statement:: tkey-gssapi-credential
   :tags: security
   :short: Sets the security credential for authentication keys requested by the GSS-TSIG protocol.

   This is the security credential with which the server should authenticate
   keys requested by the GSS-TSIG protocol. Currently only Kerberos 5
   authentication is available; the credential is a Kerberos
   principal which the server can acquire through the default system key
   file, normally ``/etc/krb5.keytab``. The location of the keytab file can be
   overridden using the :any:`tkey-gssapi-keytab` option. Normally this
   principal is of the form ``DNS/server.domain``.

.. namedconf:statement:: dump-file
   :tags: logging
   :short: Indicates the pathname of the file where the server dumps the database after :option:`rndc dumpdb`.

   This is the pathname of the file the server dumps the database to, when
   instructed to do so with :option:`rndc dumpdb`. If not specified, the
   default is ``named_dump.db``.

.. namedconf:statement:: memstatistics-file
   :tags: logging
   :short: Sets the pathname of the file where the server writes memory usage statistics on exit.

   This is the pathname of the file the server writes memory usage statistics to
   on exit. If not specified, the default is ``named.memstats``.

.. lock-file:
   :tags: obsolete
   :short: Sets the pathname of the file on which :iscman:`named` attempts to acquire a file lock when starting for the first time.

   This option has been removed and using it will cause a fatal error.

.. namedconf:statement:: pid-file
   :tags: server
   :short: Specifies the pathname of the file where the server writes its process ID.

   This is the pathname of the file the server writes its process ID in. If not
   specified, the default is |named_pid|. The PID file
   is used by programs that send signals to the running name
   server. Specifying ``pid-file none`` disables the use of a PID file;
   no file is written and any existing one is removed. Note
   that ``none`` is a keyword, not a filename, and therefore is not
   enclosed in double quotes.

.. namedconf:statement:: recursing-file
   :tags: server
   :short: Specifies the pathname of the file where the server dumps queries that are currently recursing via :option:`rndc recursing`.

   This is the pathname of the file where the server dumps the queries that are
   currently recursing, when instructed to do so with :option:`rndc recursing`.
   If not specified, the default is ``named.recursing``.

.. namedconf:statement:: statistics-file
   :tags: logging, server
   :short: Specifies the pathname of the file where the server appends statistics, when using :option:`rndc stats`.

   This is the pathname of the file the server appends statistics to, when
   instructed to do so using :option:`rndc stats`. If not specified, the
   default is ``named.stats`` in the server's current directory. The
   format of the file is described in :ref:`statsfile`.

.. namedconf:statement:: bindkeys-file
   :tags: dnssec
   :short: Specifies the pathname of a file to override the built-in trusted keys provided by :iscman:`named`.

   This is the pathname of a file to override the built-in trusted keys provided
   by :iscman:`named`. See the discussion of :any:`dnssec-validation` for
   details. This is intended for server testing.

.. namedconf:statement:: secroots-file
   :tags: dnssec
   :short: Specifies the pathname of the file where the server dumps security roots, when using :option:`rndc secroots`.

   This is the pathname of the file the server dumps security roots to, when
   instructed to do so with :option:`rndc secroots`. If not specified, the
   default is ``named.secroots``.

.. namedconf:statement:: session-keyfile
   :tags: security
   :short: Specifies the pathname of the file where a TSIG session key is written, when generated by :iscman:`named` for use by ``nsupdate -l``.

   This is the pathname of the file into which to write a TSIG session key
   generated by :iscman:`named` for use by ``nsupdate -l``. If not specified,
   the default is |session_key|. (See :ref:`dynamic_update_policies`,
   and in particular the discussion of the :any:`update-policy` statement's
   ``local`` option, for more information about this feature.)

.. namedconf:statement:: session-keyname
   :tags: security
   :short: Specifies the key name for the TSIG session key.

   This is the key name to use for the TSIG session key. If not specified, the
   default is ``local-ddns``.

.. namedconf:statement:: session-keyalg
   :tags: security
   :short: Specifies the algorithm to use for the TSIG session key.

   This is the algorithm to use for the TSIG session key. Valid values are
   hmac-sha1, hmac-sha224, hmac-sha256, hmac-sha384, hmac-sha512, and
   hmac-md5. If not specified, the default is hmac-sha256.

.. namedconf:statement:: port
   :tags: server, query
   :short: Specifies the UDP/TCP port number the server uses to receive and send DNS protocol traffic.

   This is the UDP/TCP port number the server uses to receive and send DNS
   protocol traffic. The default is 53. This option is mainly intended
   for server testing; a server using a port other than 53 is not
   able to communicate with the global DNS.

.. namedconf:statement:: tls-port
   :tags: server, query
   :short: Specifies the TCP port number the server uses to receive and send DNS-over-TLS protocol traffic.

   This is the TCP port number the server uses to receive and send
   DNS-over-TLS protocol traffic. The default is 853.

.. namedconf:statement:: https-port
   :tags: server, query
   :short: Specifies the TCP port number the server uses to receive and send DNS-over-HTTPS protocol traffic.

   This is the TCP port number the server uses to receive and send
   DNS-over-HTTPS protocol traffic. The default is 443.

.. namedconf:statement:: http-port
   :tags: server, query
   :short: Specifies the TCP port number the server uses to receive and send unencrypted DNS traffic via HTTP.

   This is the TCP port number the server uses to receive and send
   unencrypted DNS traffic via HTTP (a configuration that may be useful
   when encryption is handled by third-party software or by a reverse
   proxy).

.. namedconf:statement:: http-listener-clients
   :tags: server
   :short: Limits the number of active concurrent connections on a per-listener basis.

   This sets a hard limit on the number of active concurrent connections
   on a per-listener basis. The default value is 300; setting it to 0
   removes the quota.

.. namedconf:statement:: http-streams-per-connection
   :tags: server
   :short: Limits the number of active concurrent HTTP/2 streams on a per-connection basis.

   This sets a hard limit on the number of active concurrent HTTP/2
   streams on a per-connection basis. The default value is 100;
   setting it to 0 removes the limit. Once the limit is exceeded, the
   server finishes the HTTP session.

.. namedconf:statement:: preferred-glue
   :tags: query
   :short: Controls the order of glue records in an A or AAAA response.

   If specified, the listed type (A or AAAA) is emitted before
   other glue in the additional section of a query response. The default
   is to prefer A records when responding to queries that arrived via
   IPv4, and AAAA when responding to queries that arrived via IPv6.

.. namedconf:statement:: disable-algorithms
   :tags: dnssec
   :short: Disables DNSSEC algorithms from a specified zone.

   This disables the specified DNSSEC algorithms at and below the specified
   zone. Multiple :any:`disable-algorithms` statements are allowed. Only
   the best-match :any:`disable-algorithms` clause is used to
   determine the algorithms.

   If all supported algorithms are disabled, the zones covered by the
   :any:`disable-algorithms` setting are treated as insecure.

   Configured trust anchors in :any:`trust-anchors` (or :any:`managed-keys` or
   :any:`trusted-keys`) that match a disabled algorithm are ignored and treated
   as if they were not configured.

.. namedconf:statement:: disable-ds-digests
   :tags: dnssec, zone
   :short: Disables DS digest types from a specified zone.

   This disables the specified DS digest types at and below the specified
   name. Multiple :any:`disable-ds-digests` statements are allowed. Only
   the best-match :any:`disable-ds-digests` clause is used to
   determine the digest types.

   If all supported digest types are disabled, the zones covered by
   :any:`disable-ds-digests` are treated as insecure.

.. namedconf:statement:: dnssec-must-be-secure
   :tags: deprecated
   :short: Defines hierarchies that must or may not be secure (signed and validated).

   This option is deprecated and will be removed in a future release.

   This specifies hierarchies which must be or may not be secure (signed and
   validated). If ``yes``, then :iscman:`named` only accepts answers if
   they are secure. If ``no``, then normal DNSSEC validation applies,
   allowing insecure answers to be accepted. The specified domain
   must be defined as a trust anchor, for instance in a :any:`trust-anchors`
   statement, or ``dnssec-validation auto`` must be active.

.. namedconf:statement:: dns64
   :tags: query
   :short: Instructs :iscman:`named` to return mapped IPv4 addresses to AAAA queries when there are no AAAA records.

   This directive instructs :iscman:`named` to return mapped IPv4 addresses to
   AAAA queries when there are no AAAA records. It is intended to be
   used in conjunction with a NAT64. Each :any:`dns64` defines one DNS64
   prefix. Multiple DNS64 prefixes can be defined.

   Compatible IPv6 prefixes have lengths of 32, 40, 48, 56, 64, and 96, per
   :rfc:`6052`. Bits 64..71 inclusive must be zero, with the most significant bit
   of the prefix in position 0.

   In addition, a reverse IP6.ARPA zone is created for the prefix
   to provide a mapping from the IP6.ARPA names to the corresponding
   IN-ADDR.ARPA names using synthesized CNAMEs.

   .. namedconf:statement:: dns64-server
      :tags: server
      :short: Specifies the name of the server for :any:`dns64` zones.

   .. namedconf:statement:: dns64-contact
      :tags: server
      :short: Specifies the name of the contact for :any:`dns64` zones.

      :any:`dns64-server` and
      :any:`dns64-contact` can be used to specify the name of the server and
      contact for the zones. These can be set at the view/options
      level but not on a per-prefix basis.

      :any:`dns64` will also cause IPV4ONLY.ARPA to be created if not
      explicitly disabled using :any:`ipv4only-enable`.

   .. namedconf:statement:: clients
      :tags: query
      :short: Specifies an access control list (ACL) of clients that are affected by a given :any:`dns64` directive.

      Each :any:`dns64` supports an optional :any:`clients` ACL that determines
      which clients are affected by this directive. If not defined, it
      defaults to ``any;``.

   .. namedconf:statement:: mapped
      :tags: query
      :short: Specifies an access control list (ACL) of IPv4 addresses that are to be mapped to the corresponding A RRset in :any:`dns64`.

      Each :any:`dns64` block supports an optional :any:`mapped` ACL that selects which
      IPv4 addresses are to be mapped in the corresponding A RRset. If not
      defined, it defaults to ``any;``.

   .. namedconf:statement:: exclude
      :tags: query
      :short: Allows a list of IPv6 addresses to be ignored if they appear in a domain name's AAAA records in :any:`dns64`.

      Normally, DNS64 does not apply to a domain name that owns one or more
      AAAA records; these records are simply returned. The optional
      :any:`exclude` ACL allows specification of a list of IPv6 addresses that
      are ignored if they appear in a domain name's AAAA records;
      DNS64 is applied to any A records the domain name owns. If not
      defined, :any:`exclude` defaults to ::ffff:0.0.0.0/96.

   .. namedconf:statement:: suffix
      :tags: query
      :short: Defines trailing bits for mapped IPv4 address bits in :any:`dns64`.

      An optional :any:`suffix` can also be defined to set the bits trailing
      the mapped IPv4 address bits. By default these bits are set to
      ``::``. The bits matching the prefix and mapped IPv4 address must be
      zero.

   .. namedconf:statement:: recursive-only
      :tags: query
      :short: Toggles whether :any:`dns64` synthesis occurs only for recursive queries.

      If :any:`recursive-only` is set to ``yes``, the DNS64 synthesis only
      happens for recursive queries. The default is ``no``.

   .. namedconf:statement:: break-dnssec
      :tags: query
      :short: Enables :any:`dns64` synthesis even if the validated result would cause a DNSSEC validation failure.

      If :any:`break-dnssec` is set to ``yes``, the DNS64 synthesis happens
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

.. namedconf:statement:: resolver-use-dns64
   :tags: server
   :short: Specifies whether to apply DNS64 mappings when sending queries.

   If :any:`resolver-use-dns64` is set to ``yes``, then the IPv4-to-IPv6
   address transformations specified by the :any:`dns64` option are
   applied to IPv4 server addresses to which recursive queries are sent.
   This allows a server to perform lookups via a NAT64 connection; queries
   that would have been sent via IPv4 are instead sent to mapped IPv6
   addresses. The default is ``no``.

.. namedconf:statement:: ipv4only-enable
   :tags: query
   :short: Enables automatic IPv4 zones if a :any:`dns64` block is configured.

   This enables or disables automatic zones ``ipv4only.arpa``,
   ``170.0.0.192.in-addr.arpa``, and ``171.0.0.192.in-addr.arpa``.

   By default these zones are loaded if :any:`dns64` is configured.

.. namedconf:statement:: ipv4only-server
   :tags: server, query
   :short: Specifies the name of the server for the IPV4ONLY.ARPA zone created by :any:`dns64`.

.. namedconf:statement:: ipv4only-contact
   :tags: server
   :short: Specifies the contact for the IPV4ONLY.ARPA zone created by :any:`dns64`.

   :any:`ipv4only-server` and :any:`ipv4only-contact` can be used to specify the name
   of the server and contact for the IPV4ONLY.ARPA zone created by
   :any:`dns64`.

.. namedconf:statement:: dnssec-loadkeys-interval
   :tags: dnssec
   :short: Sets the frequency of automatic checks of the DNSSEC key repository.

   When a zone is configured with ``dnssec-policy;``, its key
   repository must be checked periodically to see whether the next step of a key
   rollover is due. The :any:`dnssec-loadkeys-interval` option
   sets the default interval of key repository checks, in minutes, in case
   the next key event cannot be calculated (e.g. because a DS record
   needs to be published).

   The default is ``60`` (1 hour), the minimum is ``1`` (1 minute), and
   the maximum is ``1440`` (24 hours); any higher value is silently
   reduced.

:namedconf:ref:`dnssec-policy`

   This specifies which key and signing policy (KASP) should be used for this
   zone. This is a string referring to a :namedconf:ref:`dnssec-policy` block.
   The default is ``none``.

.. namedconf:statement:: dnssec-update-mode
   :tags: obsolete

   This option no longer has any effect.

.. namedconf:statement:: nta-lifetime
   :tags: dnssec
   :short: Specifies the lifetime, in seconds, for negative trust anchors added via :option:`rndc nta`.

   This specifies the default lifetime, in seconds, for
   negative trust anchors added via :option:`rndc nta`.

   A negative trust anchor selectively disables DNSSEC validation for
   zones that are known to be failing because of misconfiguration, rather
   than an attack. When data to be validated is at or below an active
   NTA (and above any other configured trust anchors), :iscman:`named`
   aborts the DNSSEC validation process and treats the data as insecure
   rather than bogus. This continues until the NTA's lifetime has
   elapsed. NTAs persist across :iscman:`named` restarts.

   For convenience, TTL-style time-unit suffixes can be used to specify the NTA
   lifetime in seconds, minutes, or hours. It also accepts ISO 8601 duration
   formats.

   :any:`nta-lifetime` defaults to one hour; it cannot exceed one week.

.. namedconf:statement:: nta-recheck
   :tags: dnssec
   :short: Specifies the time interval for checking whether negative trust anchors added via :option:`rndc nta` are still necessary.

   This specifies how often to check whether negative trust anchors added via
   :option:`rndc nta` are still necessary.

   A negative trust anchor is normally used when a domain has stopped
   validating due to operator error; it temporarily disables DNSSEC
   validation for that domain. In the interest of ensuring that DNSSEC
   validation is turned back on as soon as possible, :iscman:`named`
   periodically sends a query to the domain, ignoring negative trust
   anchors, to find out whether it can now be validated. If so, the
   negative trust anchor is allowed to expire early.

   Validity checks can be disabled for an individual NTA by using
   :option:`rndc nta -f <rndc nta>`, or for all NTAs by setting :any:`nta-recheck` to zero.

   For convenience, TTL-style time-unit suffixes can be used to specify the NTA
   recheck interval in seconds, minutes, or hours. It also accepts ISO 8601
   duration formats.

   The default is five minutes. It cannot be longer than :any:`nta-lifetime`, which
   cannot be longer than a week.

.. namedconf:statement:: max-zone-ttl
   :tags: deprecated
   :short: Specifies a maximum permissible time-to-live (TTL) value, in seconds.

   This should now be configured as part of :namedconf:ref:`dnssec-policy`.
   Use of this option in :namedconf:ref:`options`, :namedconf:ref:`view`,
   and :namedconf:ref:`zone` blocks is a fatal error if
   :namedconf:ref:`dnssec-policy` has also been configured for the same
   zone. In zones without :namedconf:ref:`dnssec-policy`, this option is
   deprecated, and will be rendered non-operational in a future release.

   :any:`max-zone-ttl` specifies a maximum permissible TTL value in seconds.
   For convenience, TTL-style time-unit suffixes may be used to specify the
   maximum value. When a zone file is loaded, any record encountered with a
   TTL higher than :any:`max-zone-ttl` causes the zone to be rejected.

   This is needed in DNSSEC-maintained zones because when rolling to a new
   DNSKEY, the old key needs to remain available until RRSIG records
   have expired from caches. The :any:`max-zone-ttl` option guarantees that
   the largest TTL in the zone is no higher than the set value.

   When used in :namedconf:ref:`options`, :namedconf:ref:`view` and
   :namedconf:ref:`zone` blocks, setting :any:`max-zone-ttl` to zero
   is equivalent to "unlimited".

.. namedconf:statement:: stale-answer-ttl
   :tags: query
   :short: Specifies the time to live (TTL) to be returned on stale answers, in seconds.

   This specifies the TTL to be returned on stale answers. The default is 30
   seconds. The minimum allowed is 1 second; a value of 0 is updated silently
   to 1 second.

   For stale answers to be returned, they must be enabled, either in the
   configuration file using :any:`stale-answer-enable` or via
   :option:`rndc serve-stale on <rndc serve-stale>`.

.. namedconf:statement:: serial-update-method
   :tags: zone
   :short: Specifies the update method to be used for the zone serial number in the SOA record.

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

.. namedconf:statement:: zone-statistics
   :tags: zone, logging
   :short: Controls the level of statistics gathered for all zones.

   If ``full``, the server collects statistical data on all zones,
   unless specifically turned off on a per-zone basis by specifying
   ``zone-statistics terse`` or ``zone-statistics none`` in the :any:`zone`
   statement. The statistical data includes, for example, DNSSEC signing
   operations and the number of authoritative answers per query type. The
   default is ``terse``, providing minimal statistics on zones
   (including name and current serial number, but not query type
   counters), and also information about the currently ongoing incoming zone
   transfers.

   These statistics may be accessed via the ``statistics-channel`` or
   using :option:`rndc stats`, which dumps them to the file listed in the
   :any:`statistics-file`. See also :ref:`statsfile`.

   For backward compatibility with earlier versions of BIND 9, the
   :any:`zone-statistics` option can also accept ``yes`` or ``no``; ``yes``
   has the same meaning as ``full``. As of BIND 9.10, ``no`` has the
   same meaning as ``none``; previously, it was the same as ``terse``.

.. _boolean_options:

Boolean Options
^^^^^^^^^^^^^^^

.. namedconf:statement:: automatic-interface-scan
   :tags: server
   :short: Controls the automatic rescanning of network interfaces when addresses are added or removed.

   If ``yes`` and supported by the operating system, this automatically rescans
   network interfaces when the interface addresses are added or removed.  The
   default is ``yes``.  This configuration option does not affect the time-based
   :any:`interface-interval` option; it is recommended to set the time-based
   :any:`interface-interval` to 0 when the operator confirms that automatic
   interface scanning is supported by the operating system.

   The :any:`automatic-interface-scan` implementation uses routing sockets for the
   network interface discovery; therefore, the operating system must
   support the routing sockets for this feature to work.

.. namedconf:statement:: allow-new-zones
   :tags: server, zone
   :short: Controls the ability to add zones at runtime via :option:`rndc addzone`.

   If ``yes``, then zones can be added at runtime via :option:`rndc addzone`.
   The default is ``no``.

   Newly added zones' configuration parameters are stored so that they
   can persist after the server is restarted. The configuration
   information is saved in a file called ``viewname.nzf`` (or, if
   :iscman:`named` is compiled with liblmdb, in an LMDB database file called
   ``viewname.nzd``). "viewname" is the name of the view, unless the view
   name contains characters that are incompatible with use as a file
   name, in which case a cryptographic hash of the view name is used
   instead.

   Configurations for zones added at runtime are stored either in
   a new-zone file (NZF) or a new-zone database (NZD), depending on
   whether :iscman:`named` was linked with liblmdb at compile time. See
   :ref:`man_rndc` for further details about :option:`rndc addzone`.

.. namedconf:statement:: auth-nxdomain
   :tags: query
   :short: Controls whether BIND, acting as a resolver, provides authoritative NXDOMAIN (domain does not exist) answers.

   If ``yes``, then the ``AA`` bit is always set on NXDOMAIN responses,
   even if the server is not actually authoritative. The default is
   ``no``.

.. namedconf:statement:: memstatistics
   :tags: server, logging
   :short: Controls whether memory statistics are written to the file specified by :any:`memstatistics-file` at exit.

   This writes memory statistics to the file specified by
   :any:`memstatistics-file` at exit. The default is ``no`` unless :option:`-m
   record <named -m>` is specified on the command line, in which case it is ``yes``.

.. namedconf:statement:: dialup
   :tags: deprecated
   :short: Concentrates zone maintenance so that all transfers take place once every :any:`heartbeat-interval`, ideally during a single call.

   This option is deprecated and will be removed in a future release.

   If ``yes``, then the server treats all zones as if they are doing
   zone transfers across a dial-on-demand dialup link, which can be
   brought up by traffic originating from this server. Although this setting has
   different effects according to zone type, it concentrates the zone
   maintenance so that everything happens quickly, once every
   :any:`heartbeat-interval`, ideally during a single call. It also
   suppresses some normal zone maintenance traffic. The default
   is ``no``.

   If specified in the :any:`view` and
   :any:`zone` statements, the :any:`dialup` option overrides the global :any:`dialup`
   option.

   If the zone is a primary zone, the server sends out a NOTIFY
   request to all the secondaries (default). This should trigger the zone
   serial number check in the secondary (providing it supports NOTIFY),
   allowing the secondary to verify the zone while the connection is active.
   The set of servers to which NOTIFY is sent can be controlled by
   :namedconf:ref:`notify` and :any:`also-notify`.

   If the zone is a secondary or stub zone, the server suppresses
   the regular "zone up to date" (refresh) queries and only performs them
   when the :any:`heartbeat-interval` expires, in addition to sending NOTIFY
   requests.

   Finer control can be achieved by using :namedconf:ref:`notify`, which only sends
   NOTIFY messages; ``notify-passive``, which sends NOTIFY messages and
   suppresses the normal refresh queries; ``refresh``, which suppresses
   normal refresh processing and sends refresh queries when the
   :any:`heartbeat-interval` expires; and ``passive``, which disables
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

   Note that normal NOTIFY processing is not affected by :any:`dialup`.

.. namedconf:statement:: flush-zones-on-shutdown
   :tags: zone
   :short: Controls whether pending zone writes are flushed when the name server exits.

   When the name server exits upon receiving SIGTERM, flush or do not
   flush any pending zone writes. The default is
   ``flush-zones-on-shutdown no``.

.. namedconf:statement:: root-key-sentinel
   :tags: server
   :short: Controls whether BIND 9 responds to root key sentinel probes.

   If ``yes``, the server responds to root key sentinel probes as described in
   :rfc:`8509`:. The default is ``yes``.

.. namedconf:statement:: reuseport
   :tags: server
   :short: Enables kernel load-balancing of sockets.

   This option enables kernel load-balancing of sockets on systems which support
   it, including Linux (SO_REUSEPORT) and FreeBSD (SO_REUSEPORT_LB). This
   instructs the kernel to distribute incoming socket connections among the
   networking threads based on a hashing scheme. For more information, see the
   receive network flow classification options (``rx-flow-hash``) section in the
   ``ethtool`` manual page. The default is ``yes``.

   Enabling :any:`reuseport` significantly increases general throughput when
   incoming traffic is distributed uniformly onto the threads by the
   operating system. However, in cases where a worker thread is busy with a
   long-lasting operation, such as processing a Response Policy Zone (RPZ) or
   Catalog Zone update or an unusually large zone transfer, incoming traffic
   that hashes onto that thread may be delayed. On servers where these events
   occur frequently, it may be preferable to disable socket load-balancing so
   that other threads can pick up the traffic that would have been sent to the
   busy thread.

   Note: this option can only be set when :iscman:`named` first starts.
   Changes will not take effect during reconfiguration; the server
   must be restarted.

.. namedconf:statement:: message-compression
   :tags: query
   :short: Controls whether DNS name compression is used in responses to regular queries.

   If ``yes``, DNS name compression is used in responses to regular
   queries (not including AXFR or IXFR, which always use compression).
   Setting this option to ``no`` reduces CPU usage on servers and may
   improve throughput. However, it increases response size, which may
   cause more queries to be processed using TCP; a server with
   compression disabled is out of compliance with :rfc:`1123` Section
   6.1.3.2. The default is ``yes``.

.. namedconf:statement:: minimal-responses
   :tags: query
   :short: Controls whether the server only adds records to the authority and additional data sections when they are required (e.g. delegations, negative responses). This improves server performance.

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

   :any:`minimal-responses` takes one of four values:

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

.. namedconf:statement:: minimal-any
   :tags: query
   :short: Controls whether the server replies with only one of the RRsets for a query name, when generating a positive response to a query of type ANY over UDP.

   If set to ``yes``, the server replies with only one of
   the RRsets for the query name, and its covering RRSIGs if any,
   when generating a positive response to a query of type ANY over UDP,
   instead of replying with all known RRsets for the name. Similarly, a
   query for type RRSIG is answered with the RRSIG records covering
   only one type. This can reduce the impact of some kinds of attack
   traffic, without harming legitimate clients. (Note, however, that the
   RRset returned is the first one found in the database; it is not
   necessarily the smallest available RRset.) Additionally,
   :any:`minimal-responses` is turned on for these queries, so no
   unnecessary records are added to the authority or additional
   sections. The default is ``no``.

.. namedconf:statement:: notify
   :tags: transfer
   :short: Controls whether ``NOTIFY`` messages are sent on zone changes.

   If set to ``yes`` (the default), DNS NOTIFY messages are sent when a
   zone the server is authoritative for changes; see :ref:`using notify<notify>`.
   The messages are sent to the servers listed in the zone's NS records
   (except the primary server identified in the SOA MNAME field), and to
   any servers listed in the :any:`also-notify` option.

   If set to ``primary-only`` (or the older keyword ``master-only``),
   notifies are only sent for primary zones. If set to ``explicit``,
   notifies are sent only to servers explicitly listed using
   :any:`also-notify`. If set to ``no``, no notifies are sent.

   The :namedconf:ref:`notify` option may also be specified in the :any:`zone`
   statement, in which case it overrides the ``options notify``
   statement. It would only be necessary to turn off this option if it
   caused secondary zones to crash.

.. namedconf:statement:: notify-to-soa
   :tags: transfer
   :short: Controls whether the name servers in the NS RRset are checked against the SOA MNAME.

   If ``yes``, do not check the name servers in the NS RRset against the
   SOA MNAME. Normally a NOTIFY message is not sent to the SOA MNAME
   (SOA ORIGIN), as it is supposed to contain the name of the ultimate
   primary server. Sometimes, however, a secondary server is listed as the SOA MNAME in
   hidden primary configurations; in that case, the
   ultimate primary should be set to still send NOTIFY messages to all the name servers
   listed in the NS RRset.

.. namedconf:statement:: recursion
   :tags: query
   :short: Defines whether recursion and caching are allowed.

   If ``yes``, and a DNS query requests recursion, then the server
   attempts to do all the work required to answer the query. If recursion
   is off and the server does not already know the answer, it
   returns a referral response. The default is ``yes``. Note that setting
   ``recursion no`` does not prevent clients from getting data from the
   server's cache; it only prevents new data from being cached as an
   effect of client queries. Caching may still occur as an effect of the
   server's internal operation, such as NOTIFY address lookups.

.. namedconf:statement:: request-nsid
   :tags: query
   :short: Controls whether an empty EDNS(0) NSID (Name Server Identifier) option is sent with all queries to authoritative name servers during iterative resolution.

   If ``yes``, then an empty EDNS(0) NSID (Name Server Identifier)
   option is sent with all queries to authoritative name servers during
   iterative resolution. If the authoritative server returns an NSID
   option in its response, then its contents are logged in the ``nsid``
   category at level ``info``. The default is ``no``.

.. namedconf:statement:: require-cookie
   :tags: query
   :short: Controls whether responses without a server cookie are accepted.

   The ``require-cookie`` clause can be used to indicate that the
   remote server is known to support DNS COOKIE. Setting this option
   to ``yes`` causes :iscman:`named` to always retry a request over TCP when
   it receives a UDP response without a DNS COOKIE from the remote
   server, even if UDP responses with DNS COOKIE have not been sent
   by this server before. This prevents spoofed answers from being
   accepted without a retry over TCP, when :iscman:`named` has not yet
   determined whether the remote server supports DNS COOKIE. Setting
   this option to ``no`` (the default) causes :iscman:`named` to rely on
   autodetection of DNS COOKIE support to determine when to retry a
   request over TCP.

   For DNAME lookups the default is ``yes`` and it is enforced.  Servers
   serving DNAME must correctly support DNS over TCP.

   .. note::
      If a UDP response is signed using TSIG, :iscman:`named` accepts it even if
      ``require-cookie`` is set to ``yes`` and the response does not
      contain a DNS COOKIE.

   The ``send-cookie`` clause determines whether the local server adds
   a COOKIE EDNS option to requests sent to the server. This overrides
   ``send-cookie`` set at the view or option level. The :iscman:`named` server
   may determine that COOKIE is not supported by the remote server and not
   add a COOKIE EDNS option to requests.

.. namedconf:statement:: require-server-cookie
   :tags: query
   :short: Controls whether a valid server cookie is required before sending a full response to a UDP request.

   If ``yes``, BIND requires a valid server cookie before sending a full response to a UDP
   request from a cookie-aware client. BADCOOKIE is sent if there is a
   bad or nonexistent server cookie.

   The default is ``no``.

   Users wishing to test that DNS COOKIE clients correctly handle
   BADCOOKIE, or who are getting a lot of forged DNS requests with DNS COOKIES
   present, should set this to ``yes``. Setting this to ``yes`` results in a reduced amplification effect
   in a reflection attack, as the BADCOOKIE response is smaller than a full
   response, while also requiring a legitimate client to follow up with a second
   query with the new, valid, cookie.

.. namedconf:statement:: answer-cookie
   :tags: query
   :short: Controls whether COOKIE EDNS replies are sent in response to client queries.

   When set to the default value of ``yes``, COOKIE EDNS options are
   sent when applicable in replies to client queries. If set to ``no``,
   COOKIE EDNS options are not sent in replies. This can only be set
   at the global options level, not per-view.

   ``answer-cookie no`` is intended as a temporary measure, for use when
   :iscman:`named` shares an IP address with other servers that do not yet
   support DNS COOKIE. A mismatch between servers on the same address is
   not expected to cause operational problems, but the option to disable
   COOKIE responses so that all servers have the same behavior is
   provided out of an abundance of caution. DNS COOKIE is an important
   security mechanism, and should not be disabled unless absolutely
   necessary.

.. namedconf:statement:: send-cookie
   :tags: query
   :short: Controls whether a COOKIE EDNS option is sent along with a query.

   If ``yes``, a COOKIE EDNS option is sent along with the query.
   If the resolver has previously communicated with the server, the COOKIE
   returned in the previous transaction is sent. This is used by the
   server to determine whether the resolver has talked to it before. A
   resolver sending the correct COOKIE is assumed not to be an off-path
   attacker sending a spoofed-source query; the query is therefore
   unlikely to be part of a reflection/amplification attack, so
   resolvers sending a correct COOKIE option are not subject to response-rate
   limiting (RRL). Resolvers which do not send a correct COOKIE
   option may be limited to receiving smaller responses via the
   :any:`nocookie-udp-size` option.

   The :iscman:`named` server may determine that COOKIE is not supported by the
   remote server and not add a COOKIE EDNS option to requests.

   The default is ``yes``.

.. namedconf:statement:: stale-answer-enable
   :tags: server, query
   :short: Enables the returning of "stale" cached answers when the name servers for a zone are not answering.

   If ``yes``, this option enables the returning of "stale" cached answers when the name
   servers for a zone are not answering and the :any:`stale-cache-enable` option is
   also enabled. The default is not to return stale answers.

   Stale answers can also be enabled or disabled at runtime via
   :option:`rndc serve-stale on <rndc serve-stale>` or :option:`rndc serve-stale off <rndc serve-stale>`; these override
   the configured setting. :option:`rndc serve-stale reset <rndc serve-stale>` restores the
   setting to the one specified in :iscman:`named.conf`. Note that if stale
   answers have been disabled by :iscman:`rndc`, they cannot be
   re-enabled by reloading or reconfiguring :iscman:`named`; they must be
   re-enabled with :option:`rndc serve-stale on <rndc serve-stale>`, or the server must be
   restarted.

   Information about stale answers is logged under the ``serve-stale``
   log category.

.. namedconf:statement:: stale-answer-client-timeout
   :tags: server, query
   :short: Defines the amount of time (in milliseconds) that :iscman:`named` waits before attempting to answer a query with a stale RRset from cache.

   This option defines the amount of time (in milliseconds) that :iscman:`named`
   waits before attempting to answer the query with a stale RRset from cache.
   If a stale answer is found, :iscman:`named` continues the ongoing fetches,
   attempting to refresh the RRset in cache until the
   :any:`resolver-query-timeout` interval is reached.

   This option is off by default, which is equivalent to setting it to
   ``off`` or ``disabled``. It also has no effect if :any:`stale-answer-enable`
   is disabled.

   The minimum value, ``0``, causes a cached (stale) RRset to be
   immediately returned if it is available, while still attempting to
   refresh the data in cache.

   When this option is enabled, the only supported value in the current version
   of BIND 9 is ``0``. Non-zero values generate a warning message and are
   treated as ``0``.

.. namedconf:statement:: stale-cache-enable
   :tags: server, query
   :short: Enables the retention of "stale" cached answers.

   If ``yes``, enable the retaining of "stale" cached answers.  Default ``no``.

.. namedconf:statement:: stale-refresh-time
   :tags: server, query
   :short: Sets the time window for the return of "stale" cached answers before the next attempt to contact, if the name servers for a given zone are not responding.

   If the name servers for a given zone are not answering, this sets the time
   window for which :iscman:`named` will promptly return "stale" cached answers for
   that RRSet being requested before a new attempt in contacting the servers
   is made. For convenience, TTL-style time-unit suffixes may be used to
   specify the value. It also accepts ISO 8601 duration formats.

   The default :any:`stale-refresh-time` is 30 seconds, as :rfc:`8767` recommends
   that attempts to refresh to be done no more frequently than every 30
   seconds. A value of zero disables the feature, meaning that normal
   resolution will take place first, if that fails only then :iscman:`named` will
   return "stale" cached answers.

.. namedconf:statement:: nocookie-udp-size
   :tags: query
   :short: Sets the maximum size of UDP responses that are sent to queries without a valid server COOKIE.

   This sets the maximum size of UDP responses that are sent to queries
   without a valid server COOKIE. A value below 128 is silently
   raised to 128. The default value is 4096, but the :any:`max-udp-size`
   option may further limit the response size as the default for
   :any:`max-udp-size` is 1232.

.. namedconf:statement:: cookie-algorithm
   :tags: server
   :short: Sets the algorithm to be used when generating a server cookie.

   This sets the algorithm to be used when generating the server cookie. The
   default is "siphash24", which is the only supported option, as the
   previously supported "aes" option has been removed.

.. namedconf:statement:: cookie-secret
   :tags: server
   :short: Specifies a shared secret used for generating and verifying EDNS COOKIE options within an anycast cluster.

   If set, this is a shared secret used for generating and verifying
   EDNS COOKIE options within an anycast cluster. If not set, the system
   generates a random secret at startup. The shared secret is
   encoded as a hex string and needs to be 128 bits.

   If there are multiple secrets specified, the first one listed in
   :iscman:`named.conf` is used to generate new server cookies. The others
   are only used to verify returned cookies.

.. namedconf:statement:: response-padding
   :tags: query
   :short: Adds an EDNS Padding option to encrypted messages, to reduce the chance of guessing the contents based on size.

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

.. namedconf:statement:: trust-anchor-telemetry
   :tags: dnssec
   :short: Instructs :iscman:`named` to send specially formed queries once per day to domains for which trust anchors have been configured.

   This causes :iscman:`named` to send specially formed queries once per day to
   domains for which trust anchors have been configured via, e.g.,
   :any:`trust-anchors` or ``dnssec-validation auto``.

   The query name used for these queries has the form
   ``_ta-xxxx(-xxxx)(...).<domain>``, where each "xxxx" is a group of four
   hexadecimal digits representing the key ID of a trusted DNSSEC key.
   The key IDs for each domain are sorted smallest to largest prior to
   encoding. The query type is NULL.

   By monitoring these queries, zone operators are able to see which
   resolvers have been updated to trust a new key; this may help them
   decide when it is safe to remove an old one.

   The default is ``yes``.

.. namedconf:statement:: provide-ixfr
   :tags: transfer
   :short: Controls whether a primary responds to an incremental zone request (IXFR) or only responds with a full zone transfer (AXFR).

   The :any:`provide-ixfr` clause determines whether the local server, acting
   as primary, responds with an incremental zone transfer when the given
   remote server, a secondary, requests it. If set to ``yes``, incremental
   transfer is provided whenever possible. If set to ``no``, all
   transfers to the remote server are non-incremental.

.. namedconf:statement:: request-ixfr
   :tags: transfer
   :short: Controls whether a secondary requests an incremental zone transfer (IXFR) or a full zone transfer (AXFR).

   The :any:`request-ixfr` statement determines whether the local server, acting
   as a secondary, requests incremental zone transfers from the given
   remote server, a primary.

   IXFR requests to servers that do not support IXFR automatically
   fall back to AXFR. Therefore, there is no need to manually list which
   servers support IXFR and which ones do not; the global default of
   ``yes`` should always work. The purpose of the :any:`provide-ixfr` and
   :any:`request-ixfr` statements is to make it possible to disable the use of
   IXFR even when both primary and secondary claim to support it: for example, if
   one of the servers is buggy and crashes or corrupts data when IXFR is
   used.

   It may also be set in the zone block; if set there, it overrides the global
   or view setting for that zone. It may also be set in the
   :namedconf:ref:`server` block.

.. namedconf:statement:: request-expire
   :tags: transfer, query
   :short: Specifies whether the local server requests the EDNS EXPIRE value, when acting as a secondary.

   The :any:`request-expire` statement determines whether the local server, when
   acting as a secondary, requests the EDNS EXPIRE value. The EDNS EXPIRE
   value indicates the remaining time before the zone data expires and
   needs to be refreshed. This is used when a secondary server transfers
   a zone from another secondary server; when transferring from the
   primary, the expiration timer is set from the EXPIRE field of the SOA
   record instead. The default is ``yes``.

.. namedconf:statement:: match-mapped-addresses
   :tags: server
   :short: Allows IPv4-mapped IPv6 addresses to match address-match list entries for corresponding IPv4 addresses.

   If ``yes``, then an IPv4-mapped IPv6 address matches any
   address-match list entries that match the corresponding IPv4 address.

   This option was introduced to work around a kernel quirk in some
   operating systems that causes IPv4 TCP connections, such as zone
   transfers, to be accepted on an IPv6 socket using mapped addresses.
   This caused address-match lists designed for IPv4 to fail to match.
   However, :iscman:`named` now solves this problem internally. The use of
   this option is discouraged.

.. namedconf:statement:: ixfr-from-differences
   :tags: transfer
   :short: Controls how IXFR transfers are calculated.

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

   :any:`ixfr-from-differences` also accepts ``primary``
   and ``secondary`` at the view and options levels,
   which causes :any:`ixfr-from-differences` to be enabled for all primary
   or secondary zones, respectively. It is off for all zones by default.

   Note: if inline signing is enabled for a zone, the user-provided
   :any:`ixfr-from-differences` setting is ignored for that zone.

.. namedconf:statement:: multi-master
   :tags: transfer
   :short: Controls whether serial number mismatch errors are logged.

   This should be set when there are multiple primary servers for a zone and the
   addresses refer to different machines. If ``yes``, :iscman:`named` does not
   log when the serial number on the primary is less than what :iscman:`named`
   currently has. The default is ``no``.

.. namedconf:statement:: dnssec-validation
   :tags: dnssec
   :short: Enables DNSSEC validation in :iscman:`named`.

   This option enables DNSSEC validation in :iscman:`named`.

   If set to ``auto``, DNSSEC validation is enabled and a default trust
   anchor for the DNS root zone is used. This trust anchor is provided
   as part of BIND and is kept up-to-date using :ref:`rfc5011.support` key
   management. Adding an explicit static key using the :any:`trust-anchors`
   statement, with a ``static-key`` anchor type (or using the deprecated
   :any:`trusted-keys` statement) for the root zone, is not supported with the
   ``auto`` setting and is treated as a configuration error.

   If set to ``yes``, DNSSEC validation is enabled, but a trust anchor must be
   manually configured using a :any:`trust-anchors` statement (or the
   :any:`managed-keys` or :any:`trusted-keys` statements, both deprecated). If
   :any:`trust-anchors` is not configured, it is a configuration error. If
   :any:`trust-anchors` does not include a valid root key, then validation does
   not take place for names which are not covered by any of the configured trust
   anchors.

   If set to ``no``, DNSSEC validation is disabled. (Note: the resolver
   will still set the DO bit in outgoing queries to indicate that it can
   accept DNSSEC responses, even if :any:`dnssec-validation` is disabled.)

   The default is ``auto``, unless BIND is built with
   ``configure --disable-auto-validation``, in which case the default is
   ``yes``.

   The default root trust anchor is compiled into :iscman:`named`
   and is current as of the release date. If the root key changes, a
   running BIND server detects this and rolls smoothly to the new
   key. However, newly installed servers will be unable to start validation,
   and BIND must be upgraded to a newer version.

.. namedconf:statement:: validate-except
   :tags: dnssec
   :short: Specifies a list of domain names at and beneath which DNSSEC validation should not be performed.

   This specifies a list of domain names at and beneath which DNSSEC
   validation should *not* be performed, regardless of the presence of a
   trust anchor at or above those names. This may be used, for example,
   when configuring a top-level domain intended only for local use, so
   that the lack of a secure delegation for that domain in the root zone
   does not cause validation failures. (This is similar to setting a
   negative trust anchor except that it is a permanent configuration,
   whereas negative trust anchors expire and are removed after a set
   period of time.)

.. namedconf:statement:: dnssec-accept-expired
   :tags: dnssec
   :short: Instructs BIND 9 to accept expired DNSSEC signatures when validating.

   This accepts expired signatures when verifying DNSSEC signatures. The
   default is ``no``. Setting this option to ``yes`` leaves :iscman:`named`
   vulnerable to replay attacks.

.. namedconf:statement:: querylog
   :tags: logging, server
   :short: Specifies whether query logging should be active when :iscman:`named` first starts.

   Query logging provides a complete log of all incoming queries and all query
   errors. This provides more insight into the server's activity, but with a
   cost to performance which may be significant on heavily loaded servers.

   The :any:`querylog` option specifies whether query logging should be active when
   :iscman:`named` first starts. If :any:`querylog` is not specified, then query logging
   is determined by the presence of the logging category ``queries``.  Please
   note that :option:`rndc reconfig` and :option:`rndc reload` have no effect on
   this option, so it cannot be changed once the server is running. However,
   query logging can be activated at runtime using the command ``rndc querylog
   on``, or deactivated with :option:`rndc querylog off <rndc querylog>`.

.. namedconf:statement:: responselog
   :tags: logging, server
   :short: Specifies whether response logging should be active when :iscman:`named` first starts.

   Response logging complements :any:`querylog` by logging the rcode of
   previous queries along with the queries' name, type and class.

   Response logging can also be activated at runtime using the
   command ``rndc responselog on``, or deactivated with :option:`rndc
   responselog off <rndc responselog>`.

.. namedconf:statement:: check-names
   :tags: query, server
   :short: Restricts the character set and syntax of certain domain names in primary files and/or DNS responses received from the network.

   This option is used to restrict the character set and syntax of
   certain domain names in primary files and/or DNS responses received
   from the network. The default varies according to usage area. For
   :any:`type primary` zones the default is ``fail``. For :any:`type secondary` zones the
   default is ``warn``. For answers received from the network
   (``response``), the default is ``ignore``.

   The rules for legal hostnames and mail domains are derived from
   :rfc:`952` and :rfc:`821` as modified by :rfc:`1123`.

   :any:`check-names` applies to the owner names of A, AAAA, and MX records.
   It also applies to the domain names in the RDATA of NS, SOA, MX, and
   SRV records. It further applies to the RDATA of PTR records where the
   owner name indicates that it is a reverse lookup of a hostname (the
   owner name ends in IN-ADDR.ARPA, IP6.ARPA, or IP6.INT).

.. namedconf:statement:: check-dup-records
   :tags: dnssec, query
   :short: Checks primary zones for records that are treated as different by DNSSEC but are semantically equal in plain DNS.

   This checks primary zones for records that are treated as different by
   DNSSEC but are semantically equal in plain DNS. The default is to
   ``warn``. Other possible values are ``fail`` and ``ignore``.

.. namedconf:statement:: check-mx
   :tags: zone
   :short: Checks whether an MX record appears to refer to an IP address.

   This checks whether the MX record appears to refer to an IP address. The
   default is to ``warn``. Other possible values are ``fail`` and
   ``ignore``.

.. namedconf:statement:: check-wildcard
   :tags: zone
   :short: Checks for non-terminal wildcards.

   This option is used to check for non-terminal wildcards. The use of
   non-terminal wildcards is almost always as a result of a lack of
   understanding of the wildcard-matching algorithm (:rfc:`1034`). This option
   affects primary zones. The default (``yes``) is to check for
   non-terminal wildcards and issue a warning.

.. namedconf:statement:: check-integrity
   :tags: zone
   :short: Performs post-load zone integrity checks on primary zones.

   This performs post-load zone integrity checks on primary zones. It checks
   that MX and SRV records refer to address (A or AAAA) records and that
   glue address records exist for delegated zones. For MX and SRV
   records, only in-zone hostnames are checked (for out-of-zone hostnames,
   use :iscman:`named-checkzone`). For NS records, only names below top-of-zone
   are checked (for out-of-zone names and glue consistency checks, use
   :iscman:`named-checkzone`). DS records not at delegations are rejected.
   The default is ``yes``.

   The use of the SPF record to publish Sender Policy Framework is
   deprecated, as the migration from using TXT records to SPF records was
   abandoned. Enabling this option also checks that a TXT Sender Policy
   Framework record exists (starts with "v=spf1") if there is an SPF
   record. Warnings are emitted if the TXT record does not exist; they can
   be suppressed with :any:`check-spf`.

.. namedconf:statement:: check-mx-cname
   :tags: zone
   :short: Sets the response to MX records that refer to CNAMEs.

   If :any:`check-integrity` is set, :iscman:`named` fails, warns, or ignores MX records
   that refer to CNAMEs. The default is to ``warn``.

.. namedconf:statement:: check-srv-cname
   :tags: zone
   :short: Sets the response to SRV records that refer to CNAMEs.

   If :any:`check-integrity` is set, :iscman:`named` fails, warns, or ignores SRV records
   that refer to CNAMEs. The default is to ``warn``.

.. namedconf:statement:: check-sibling
   :tags: zone
   :short: Specifies whether to check for sibling glue when performing integrity checks.

   This option instructs BIND to also check that sibling glue exists,
   when performing integrity checks. The default is ``yes``.

.. namedconf:statement:: check-spf
   :tags: zone
   :short: Specifies whether to check for a TXT Sender Policy Framework record, if an SPF record is present.

   If :any:`check-integrity` is set, :iscman:`named` checks whether there is a TXT Sender
   Policy Framework record present (starts with "v=spf1"), if there is an
   SPF record present. The default is ``warn``.

.. namedconf:statement:: check-svcb
   :tags: zone
   :short: Specifies whether to perform additional checks on SVCB records.

   If ``yes``, :iscman:`named` checks that SVCB records that start with a ``_dns``
   label prefixed by an optional ``_<port>`` label (e.g.
   ``_443._dns.ns1.example``) have an ``alpn`` parameter, and that
   the ``dohpath`` parameter exists when the ``alpn`` indicates
   that it should be present.  The default is ``yes``.

.. namedconf:statement:: zero-no-soa-ttl
   :tags: zone, query, server
   :short: Specifies whether to set the time to live (TTL) of the SOA record to zero, when returning authoritative negative responses to SOA queries.

   If ``yes``, when returning authoritative negative responses to SOA queries, :iscman:`named` sets
   the TTL of the SOA record returned in the authority section to zero.
   The default is ``yes``.

.. namedconf:statement:: zero-no-soa-ttl-cache
   :tags: zone, query, server
   :short: Sets the time to live (TTL) to zero when caching a negative response to an SOA query.

   If ``yes``, this option instructs BIND to set the TTL to zero when caching a negative response to an SOA query.
   The default is ``no``.

.. namedconf:statement:: update-check-ksk
   :tags: obsolete

   This option no longer has any effect.

.. namedconf:statement:: dnssec-dnskey-kskonly
   :tags: obsolete

   This option no longer has any effect.

.. namedconf:statement:: try-tcp-refresh
   :tags: transfer
   :short: Specifies that BIND 9 should attempt to refresh a zone using TCP if UDP queries fail.

   If ``yes``, BIND tries to refresh the zone using TCP if UDP queries fail. The default is
   ``yes``.

.. namedconf:statement:: dnssec-secure-to-insecure
   :tags: obsolete

   This option no longer has any effect.

.. namedconf:statement:: synth-from-dnssec
   :tags: dnssec
   :short: Enables support for :rfc:`8198`, Aggressive Use of DNSSEC-Validated Cache.

   This option enables support for :rfc:`8198`, Aggressive Use of
   DNSSEC-Validated Cache. It allows the resolver to send a smaller number
   of queries when resolving queries for DNSSEC-signed domains
   by synthesizing answers from cached NSEC and other RRsets that
   have been proved to be correct using DNSSEC.
   The default is ``yes``.

   .. note:: DNSSEC validation must be enabled for this option to be effective.
      This initial implementation only covers synthesis of answers from
      NSEC records; synthesis from NSEC3 is planned for the future. This
      will also be controlled by :any:`synth-from-dnssec`.

Forwarding
^^^^^^^^^^

The forwarding facility sends queries which cannot be answered using local data
to different resolvers. This can be used to create a large site-wide cache on
a few servers, reducing traffic over links to external name servers. It
can also be used to allow queries by servers that do not have direct
access to the Internet, but that wish to look up exterior names anyway.
Forwarding occurs only on those queries for which the server is not
authoritative and does not have the answer in its cache.

.. namedconf:statement:: forward
   :tags: query
   :short: Allows or disallows fallback to recursion if forwarding has failed; it is always used in conjunction with the :any:`forwarders` statement.

   This option is only meaningful if the :any:`forwarders` list is not empty. A
   value of ``first`` is the default and causes the server to query the
   forwarders first; if that does not answer the question, the
   server then looks for the answer itself. If ``only`` is
   specified, the server only queries the forwarders.

.. namedconf:statement:: forwarders
   :tags: query
   :short: Defines one or more resolvers to which queries are forwarded.

   This specifies a list of IP addresses of DNS resolvers, to which queries
   which cannot be answered using locally available data are forwarded. The
   default is the empty list (no forwarding). Each address in the list can be
   associated with an optional port number and a TLS transport. A default port
   number and a TLS transport can be set for the entire list.

   If a TLS configuration is specified, :iscman:`named` uses DNS-over-TLS
   (DoT) connections when connecting to the specified IP address(es), via the
   TLS configuration referenced by the :any:`tls` statement.

Forwarding can also be configured on a per-domain basis, allowing for
the global forwarding options to be overridden in a variety of ways.
Particular domains can be set to use different forwarders, or have a
different ``forward only/first`` behavior, or not forward at all; see
:any:`zone`.

.. _dual_stack:

Dual-stack Servers
^^^^^^^^^^^^^^^^^^

Dual-stack servers are used as servers of last resort, to work around
problems in reachability due to the lack of support for either IPv4 or IPv6
on the host machine.

.. namedconf:statement:: dual-stack-servers
   :tags: server
   :short: Specifies host names or addresses of machines with access to both IPv4 and IPv6 transports.

   This specifies host names or addresses of machines with access to both
   IPv4 and IPv6 transports. If a hostname is used, the server must be
   able to resolve the name using only the transport it has. If the
   machine is dual-stacked, the :any:`dual-stack-servers` parameter has no
   effect unless access to a transport has been disabled on the command
   line (e.g., :option:`named -4`).

.. _access_control:

Access Control
^^^^^^^^^^^^^^

Access to the server can be restricted based on the IP address of the
requesting system. See :ref:`address_match_lists`
for details on how to specify IP address lists.

.. namedconf:statement:: allow-notify
   :tags: transfer
   :short: Defines an :any:`address_match_list` that is allowed to send ``NOTIFY`` messages for the zone, in addition to addresses defined in the :any:`primaries` option for the zone.

   This ACL specifies which hosts may send NOTIFY messages to inform
   this server of changes to zones for which it is acting as a secondary
   server. This is only applicable for secondary zones (i.e., :any:`type
   secondary` or ``slave``).

   If this option is set in :any:`view` or :namedconf:ref:`options`, it is globally
   applied to all secondary zones. If set in the :any:`zone` statement, the
   global value is overridden.

   If not specified, the default is to process NOTIFY messages only from
   the configured :any:`primaries` for the zone. :any:`allow-notify` can be used
   to expand the list of permitted hosts, not to reduce it.

.. namedconf:statement:: allow-proxy
   :tags: server
   :short: Defines an :any:`address_match_list` for the client addresses allowed to send PROXYv2 headers.

   The default :any:`address_match_list` is `none`, which means that
   no client is allowed to do that by default for security reasons, as
   the PROXYv2 protocol provides an easy way to spoof both source and
   destination addresses.

   This :any:`address_match_list` is primarily meant to have addresses
   and subnets of the proxies that are allowed to send PROXYv2 headers
   to BIND. In most cases, we do not recommend setting this
   :any:`address_match_list` to be very permissive; in particular, we recommend against
   setting it to `any`, especially in cases when PROXYv2 headers can be
   accepted on publicly available networking interfaces.

   The specified option is the only option that matches against real
   peer addresses when PROXYv2 headers are used. Most of the options
   that work with peer addresses use the ones extracted from PROXYv2
   headers.

   See also: :namedconf:ref:`allow-proxy-on`.

.. namedconf:statement:: allow-proxy-on
   :tags: server
   :short: Defines an :any:`address_match_list` for the interface addresses allowed to accept PROXYv2 headers. The option is mostly intended for multi-homed configurations.

   The default :any:`address_match_list` is `any`, which means that
   accepting PROXYv2 is allowed on any interface.

   The option is useful in cases when a user needs to have precise control
   over which interfaces allow PROXYv2, as it is the only option
   that matches against real interface addresses when PROXYv2 headers
   are used. Most options that work with interface addresses
   use the ones extracted from PROXYv2 headers.

   It may be desirable to first set :namedconf:ref:`allow-proxy`.

.. namedconf:statement:: allow-query
   :tags: query
   :short: Specifies which hosts (an IP address list) are allowed to send queries to this resolver.

   :any:`allow-query` may also be specified in the :any:`zone` statement, in
   which case it overrides the ``options allow-query`` statement. If not
   specified, the default is to allow queries from all hosts.

   .. note:: :any:`allow-query-cache` is used to specify access to the cache.

.. namedconf:statement:: allow-query-on
   :tags: query
   :short: Specifies which local addresses (an IP address list) are allowed to send queries to this resolver. This option is used in multi-homed configurations.

   This makes it possible, for instance, to allow queries on
   internal-facing interfaces but disallow them on external-facing ones,
   without necessarily knowing the internal network's addresses.

   Note that :any:`allow-query-on` is only checked for queries that are
   permitted by :any:`allow-query`. A query must be allowed by both ACLs,
   or it is refused.

   :any:`allow-query-on` may also be specified in the :any:`zone` statement,
   in which case it overrides the ``options allow-query-on`` statement.

   If not specified, the default is to allow queries on all addresses.

   .. note:: :any:`allow-query-cache` is used to specify access to the cache.

.. namedconf:statement:: allow-query-cache
   :tags: query
   :short: Specifies which hosts (an IP address list) can access this server's cache and thus effectively controls recursion.

   This option defines an :term:`address_match_list` of IP address(es) which are allowed to
   issue queries that access the local cache. Without access to the local
   cache, recursive queries are effectively useless so, in effect, this
   statement (or its default) controls recursive behavior. This statement's
   default setting depends on:

   1. If :namedconf:ref:`recursion no; <recursion>` present, it defaults to
      ``allow-query-cache {none;};``. No local cache access permitted.

   2. If :namedconf:ref:`recursion yes; <recursion>` (default), then, if
      :any:`allow-recursion` is present, it defaults to the value of
      :any:`allow-recursion`. Local cache access is permitted to the same
      :term:`address_match_list` as :any:`allow-recursion`.

   3. If :namedconf:ref:`recursion yes; <recursion>` (default), then, if
      :any:`allow-recursion` is **not** present, it defaults to
      ``allow-query-cache {localnets; localhost;};``. Local cache access is permitted
      to :term:`address_match_list` localnets and localhost IP addresses only.

.. namedconf:statement:: allow-query-cache-on
   :tags: query
   :short: Specifies which hosts (from an IP address list) can access this server's cache. It is used on servers with multiple interfaces.

   This specifies which local addresses can send answers from the cache. If
   :any:`allow-query-cache-on` is not set, then :any:`allow-recursion-on` is
   used if set. Otherwise, the default is to allow cache responses to be
   sent from any address. Note: both :any:`allow-query-cache` and
   :any:`allow-query-cache-on` must be satisfied before a cache response
   can be sent; a client that is blocked by one cannot be allowed by the
   other.

.. namedconf:statement:: allow-recursion
   :tags: query
   :short: Defines an :any:`address_match_list` of clients that are allowed to perform recursive queries.

   This specifies which hosts are allowed to make recursive queries through
   this server. BIND checks to see if the following parameters are set, in
   order: :any:`allow-query-cache` and :any:`allow-query`. If neither of those parameters
   is set, the default (localnets; localhost;) is used.

.. namedconf:statement:: allow-recursion-on
   :tags: query, server
   :short: Specifies which local addresses can accept recursive queries.

   This specifies which local addresses can accept recursive queries. If
   :any:`allow-recursion-on` is not set, then :any:`allow-query-cache-on` is
   used if set; otherwise, the default is to allow recursive queries on
   all addresses. Any client permitted to send recursive queries can
   send them to any address on which :iscman:`named` is listening. Note: both
   :any:`allow-recursion` and :any:`allow-recursion-on` must be satisfied
   before recursion is allowed; a client that is blocked by one cannot
   be allowed by the other.

.. namedconf:statement:: allow-update
   :tags: transfer
   :short: Defines an :any:`address_match_list` of hosts that are allowed to submit dynamic updates for primary zones.

   This provides a simple access control list.
   When set in the :any:`zone` statement for a primary zone, this specifies which
   hosts are allowed to submit dynamic DNS updates to that zone. The
   default is to deny updates from all hosts.

   Note that allowing updates based on the requestor's IP address is
   insecure; see :ref:`dynamic_update_security` for details.

   In general, this option should only be set at the :any:`zone` level.
   While a default value can be set at the :namedconf:ref:`options` or :any:`view` level
   and inherited by zones, this could lead to some zones unintentionally
   allowing updates.

   Updates are written to the zone's filename that is set in :any:`file`.

.. namedconf:statement:: allow-update-forwarding
   :tags: transfer
   :short: Defines an :any:`address_match_list` of hosts that are allowed to submit dynamic updates to a secondary server for transmission to a primary.

   When set in the :any:`zone` statement for a secondary zone, this specifies which
   hosts are allowed to submit dynamic DNS updates and have them be
   forwarded to the primary. The default is ``{ none; }``, which means
   that no update forwarding is performed.

   To enable update forwarding, specify
   ``allow-update-forwarding { any; };`` in the :any:`zone` statement.
   Specifying values other than ``{ none; }`` or ``{ any; }`` is usually
   counterproductive; the responsibility for update access control
   should rest with the primary server, not the secondary.

   Note that enabling the update forwarding feature on a secondary server
   may expose primary servers to attacks if they rely on insecure
   IP-address-based access control; see :ref:`dynamic_update_security` for more details.

   In general this option should only be set at the :any:`zone` level.
   While a default value can be set at the :namedconf:ref:`options` or :any:`view` level
   and inherited by zones, this can lead to some zones unintentionally
   forwarding updates.

.. namedconf:statement:: allow-transfer
   :tags: transfer
   :short: Defines an :any:`address_match_list` of hosts that are allowed to transfer the zone information from this server.

   This specifies which hosts are allowed to receive zone transfers from the
   server. :any:`allow-transfer` may also be specified in the :any:`zone`
   statement, in which case it overrides the :any:`allow-transfer`
   statement set in :namedconf:ref:`options` or :any:`view`.

   Transport-level limitations can also be specified. In particular,
   zone transfers can be restricted to a specific port and/or DNS
   transport protocol by using the options :term:`port` and ``transport``.
   Either option can be specified; if both are used, both constraints
   must be satisfied in order for the transfer to be allowed. Zone
   transfers are currently only possible via the TCP and TLS transports.

   For example: ``allow-transfer port 853 transport tls { any; };``
   allows outgoing zone transfers to any host using the TLS transport
   over port 853.

   If :any:`allow-transfer` is not specified, then the default is
   ``none``; outgoing zone transfers are disabled.

.. warning::

   Please note that incoming TLS connections are
   **not authenticated at the TLS level by default**.
   Please use :ref:`tsig` to authenticate requestors
   or consider implementing :ref:`Mutual TLS <mutual-tls>`
   authentication.

.. namedconf:statement:: blackhole
   :tags: query
   :short: Defines an :any:`address_match_list` of hosts to ignore. The server will neither respond to queries from nor send queries to these addresses.

   This specifies a list of addresses which the server does not accept queries
   from or or cannot use to resolve a query. Queries from these addresses are not
   responded to. The default is ``none``.

.. namedconf:statement:: no-case-compress
   :tags: server
   :short: Specifies a list of addresses that require case-insensitive compression in responses.

   This specifies a list of addresses which require responses to use
   case-insensitive compression. This ACL can be used when :iscman:`named`
   needs to work with clients that do not comply with the requirement in
   :rfc:`1034` to use case-insensitive name comparisons when checking for
   matching domain names.

   If left undefined, the ACL defaults to ``none``: case-sensitive
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

   There are circumstances in which :iscman:`named` does not preserve the case
   of owner names of records: if a zone file defines records of
   different types with the same name, but the capitalization of the
   name is different (e.g., "www.example.com/A" and
   "WWW.EXAMPLE.COM/AAAA"), then all responses for that name use
   the *first* version of the name that was used in the zone file. This
   limitation may be addressed in a future release. However, domain
   names specified in the rdata of resource records (i.e., records of
   type NS, MX, CNAME, etc.) always have their case preserved unless
   the client matches this ACL.

.. namedconf:statement:: resolver-query-timeout
   :tags: query
   :short: Specifies the length of time, in milliseconds, that a resolver attempts to resolve a recursive query before failing.

   This is the amount of time, in milliseconds, that the resolver spends
   attempting to resolve a recursive query before failing. The default
   is ``10000``, the minimum is ``301``, and the maximum is ``30000``.
   Setting it to ``0`` results in the default being used.

   This value was originally specified in seconds. Values less than or
   equal to 300 are treated as seconds and converted to
   milliseconds before applying the above limits.

.. _interfaces:

Interfaces
^^^^^^^^^^

The interfaces, ports, and protocols that the server can use to answer
queries may be specified using the :any:`listen-on` and :any:`listen-on-v6` options.

.. namedconf:statement:: listen-on
   :tags: server
   :short: Specifies the IPv4 addresses on which a server listens for DNS queries.

.. namedconf:statement:: listen-on-v6
   :tags: server
   :short: Specifies the IPv6 addresses on which a server listens for DNS queries.

   The :any:`listen-on` and :any:`listen-on-v6` statements can each
   take an optional port, PROXYv2 support switch, TLS configuration
   identifier, and/or HTTP configuration identifier, in addition to an
   :term:`address_match_list`.

   The :term:`address_match_list` in :any:`listen-on` specifies the IPv4 addresses
   on which the server will listen. (IPv6 addresses are ignored, with a
   logged warning.) The server listens on all interfaces allowed by the
   address match list.  If no :any:`listen-on` is specified, the default is
   to listen for standard DNS queries on port 53 of all IPv4 interfaces.

   :any:`listen-on-v6` takes an :term:`address_match_list` of IPv6 addresses.
   The server listens on all interfaces allowed by the address match list.
   If no :any:`listen-on-v6` is specified, the default is to listen for standard
   DNS queries on port 53 of all IPv6 interfaces.

   When specified, the PROXYv2 support switch ``proxy`` allows
   the enabling of PROXYv2 protocol support. The PROXYv2 protocol
   provides the means for passing connection information, such as a
   client's source and destination addresses and ports, across
   multiple layers of NAT or TCP/UDP proxies to back-end servers. The
   addresses passed by the PROXYv2 protocol are then used, instead
   of the peer and interface addresses provided by the operating
   system.

   The ``proxy`` switch can have the following values:

   * ``plain`` - accept plain PROXYv2 headers. This is the only valid
     option for transports that do not employ encryption. In the case
     of transports that employ encryption, this value instructs BIND that
     PROXYv2 headers are sent without encryption before the TLS
     handshake. In that case, only PROXYv2 headers are not encrypted.
   * ``encrypted`` - accept encrypted PROXYv2 headers. This value
     instructs BIND that PROXYv2 headers are sent encrypted immediately
     after the TLS handshake. The option is valid only for transports
     that employ encryption; encrypted PROXYv2 headers cannot be sent
     via unencrypted transports.

   Please consult the documentation of any proxying front-end software to
   decide which value should be used. If in doubt, use ``plain`` for
   encrypted transports, especially for DNS-over-HTTPS (DoH), but
   DNS-specific software is likely to need ``encrypted``.

   It should be noted that when PROXYv2 is enabled on a listener, it
   loses the ability to accept regular DNS queries without associated
   PROXYv2 headers.

   In some cases, PROXYv2 headers might not contain usable source and
   destination addresses. In particular, this can happen when the headers
   use the ``LOCAL`` command, or headers use address types that are unspecified or
   unsupported by BIND. If otherwise correct, such
   headers are accepted by BIND and the real endpoint addresses are
   used in these cases.

   The PROXYv2 protocol is designed to be extensible and can carry
   additional information in the form of type-length-values
   (TLVs). Many of the types are defined in the protocol
   specification, and for some of these, BIND does a reasonable amount of
   validation in order to detect and reject ill-formed or hand-crafted
   headers. Apart from that, this additional data, while accepted, is
   not currently used by BIND for anything else.

   By default, no client is allowed to send queries that contain
   PROXYv2 protocol headers, even when support for the protocol is
   enabled in a :any:`listen-on` statement. Users who are interested in
   enabling the PROXYv2 protocol support may also want to
   look at the :namedconf:ref:`allow-proxy` and
   :namedconf:ref:`allow-proxy-on` options, to adjust the corresponding
   ACLs.

   If a TLS configuration is specified, :iscman:`named` will listen for DNS-over-TLS
   (DoT) connections, using the key and certificate specified in the
   referenced :any:`tls` statement. If the name ``ephemeral`` is used,
   an ephemeral key and certificate created for the currently running
   :iscman:`named` process will be used.

   If an HTTP configuration is specified, :iscman:`named` listens for
   DNS-over-HTTPS (DoH) connections using the HTTP endpoint specified in the
   referenced :any:`http` statement. If the name ``default`` is used, then
   :iscman:`named` listens for connections at the default endpoint,
   ``/dns-query``.

   Use of an :any:`http` specification requires :any:`tls` to be specified
   as well. If an unencrypted connection is desired (for example,
   on load-sharing servers behind a reverse proxy), ``tls none`` may be used.

   If a port number is not specified, the default is 53 for standard DNS,
   853 for DNS over TLS, 443 for DNS over HTTPS, and 80 for
   DNS over HTTP (unencrypted). These defaults may be overridden using the
   :namedconf:ref:`port`, :any:`tls-port`, :any:`https-port`, and :any:`http-port` options.

   Multiple :any:`listen-on` statements are allowed. For example:

   ::

      listen-on { 5.6.7.8; };
      listen-on port 1234 { !1.2.3.4; 1.2/16; };
      listen-on port 8853 tls ephemeral { 4.3.2.1; };
      listen-on port 8453 tls ephemeral http myserver { 8.7.6.5; };
      listen-on port 5300 proxy plain { !1.2.3.4; 1.2/16; };
      listen-on port 8953 proxy encrypted tls ephemeral { 4.3.2.1; };
      listen-on port 8553 proxy plain tls ephemeral http myserver { 8.7.6.5; };

   The first two lines instruct the name server to listen for standard DNS
   queries on port 53 of the IP address 5.6.7.8 and on port 1234 of an address
   on the machine in net 1.2 that is not 1.2.3.4. The third line instructs the
   server to listen for DNS-over-TLS connections on port 8853 of the IP
   address 4.3.2.1 using the ephemeral key and certifcate. The fourth line
   enables DNS-over-HTTPS connections on port 8453 of address 8.7.6.5, using
   the ephemeral key and certificate, and the HTTP endpoint or endpoints
   configured in an :any:`http` statement with the name ``myserver``.

   Multiple :any:`listen-on-v6` options can be used. For example:

   ::

      listen-on-v6 { any; };
      listen-on-v6 port 1234 { !2001:db8::/32; any; };
      listen-on-v6 port 8853 tls example-tls { 2001:db8::100; };
      listen-on-v6 port 8453 tls example-tls http default { 2001:db8::100; };
      listen-on-v6 port 8000 tls none http myserver { 2001:db8::100; };
      listen-on-v6 port 53000 proxy plain { !2001:db8::/32; any; };
      listen-on-v6 port 8953 proxy encrypted tls example-tls { 2001:db8::100; };
      listen-on-v6 port 8553 proxy plain tls example-tls http default { 2001:db8::100; };

   The first two lines instruct the name server to listen for standard DNS
   queries on port 53 of any IPv6 addresses, and on port 1234 of IPv6
   addresses that are not in the prefix 2001:db8::/32. The third line
   instructs the server to listen for for DNS-over-TLS connections on port
   8853 of the address 2001:db8::100, using a TLS key and certificate specified
   in the a :any:`tls` statement with the name ``example-tls``. The fourth
   instructs the server to listen for DNS-over-HTTPS connections, again using
   ``example-tls``, on the default HTTP endpoint. The fifth line, in which
   the :any:`tls` parameter is set to ``none``, instructs the server to listen
   for *unencrypted* DNS queries over HTTP at the endpoint specified in
   ``myserver``..

   To instruct the server not to listen on any IPv6 addresses, use:

   ::

      listen-on-v6 { none; };

.. _query_address:

Query Address
^^^^^^^^^^^^^

.. namedconf:statement:: query-source
   :tags: query
   :short: Controls the IPv4 address from which queries are issued. If
           `none`, then no IPv4 address would be used to issue the
           query and therefore only IPv6 servers are queried.

.. namedconf:statement:: query-source-v6
   :tags: query
   :short: Controls the IPv6 address from which queries are issued. If
           `none`, then no IPv6 address would be used to issue the
           query and therefore only IPv4 servers are quried.

   If the server does not know the answer to a question, it queries other
   name servers. :any:`query-source` specifies the address and port used for
   such queries. For queries sent over IPv6, there is a separate
   :any:`query-source-v6` option. If ``address`` is ``*`` (asterisk) or is
   omitted, a wildcard IP address (``INADDR_ANY``) is used.

   The defaults of the :any:`query-source` and :any:`query-source-v6` options
   are:

   ::

      query-source address * port *;
      query-source-v6 address * port *;

   .. note:: ``port`` configuration is deprecated. A warning will be logged
      when this parameter is used.

   .. note:: The address specified in the :any:`query-source` option is
      used for both UDP and TCP queries, but the port applies only to UDP
      queries. TCP queries always use a random unprivileged port.

.. namedconf:statement:: use-v4-udp-ports
   :tags: deprecated
   :short: Specifies a list of ports that are valid sources for UDP/IPv4 messages.

.. namedconf:statement:: use-v6-udp-ports
   :tags: deprecated
   :short: Specifies a list of ports that are valid sources for UDP/IPv6 messages.

   These statements, which are deprecated and will be removed in a future
   release, specify a list of IPv4 and IPv6 UDP ports that are used as
   source ports for UDP messages.

   If :term:`port` is ``*`` or is omitted, a random port number from a
   pre-configured range is selected and used for each query. The
   port range(s) are specified in the :any:`use-v4-udp-ports` (for IPv4)
   and :any:`use-v6-udp-ports` (for IPv6) options.

   If :any:`use-v4-udp-ports` or :any:`use-v6-udp-ports` is unspecified,
   :iscman:`named` checks whether the operating system provides a programming
   interface to retrieve the system's default range for ephemeral ports. If
   such an interface is available, :iscman:`named` uses the corresponding
   system default range; otherwise, it uses its own defaults:

   ::

      use-v4-udp-ports { range 1024 65535; };
      use-v6-udp-ports { range 1024 65535; };

.. namedconf:statement:: avoid-v4-udp-ports
   :tags: deprecated
   :short: Specifies the range(s) of ports to be excluded from use as sources for UDP/IPv4 messages.

.. namedconf:statement:: avoid-v6-udp-ports
   :tags: deprecated
   :short: Specifies the range(s) of ports to be excluded from use as sources for UDP/IPv6 messages.

   These statements, which are deprecated and will be removed in a future
   release, indicate ranges of port numbers to exclude from those specified
   in the :any:`avoid-v4-udp-ports` and :any:`avoid-v6-udp-ports`
   options, respectively.

   The defaults of the :any:`avoid-v4-udp-ports` and :any:`avoid-v6-udp-ports`
   options are:

   ::

      avoid-v4-udp-ports {};
      avoid-v6-udp-ports {};

   For example, with the following configuration:

   ::

      use-v6-udp-ports { range 32768 65535; };
      avoid-v6-udp-ports { 40000; range 50000 60000; };

   UDP ports of IPv6 messages sent from :iscman:`named` are in one of the
   following ranges: 32768 to 39999, 40001 to 49999, or 60001 to 65535.

   :any:`avoid-v4-udp-ports` and :any:`avoid-v6-udp-ports` can be used to prevent
   :iscman:`named` from choosing as its random source port a port that is blocked
   by a firewall or that is used by other applications; if a
   query went out with a source port blocked by a firewall, the answer
   would not pass through the firewall and the name server would have to query
   again. Note: the desired range can also be represented only with
   :any:`use-v4-udp-ports` and :any:`use-v6-udp-ports`, and the ``avoid-``
   options are redundant in that sense; they are provided for backward
   compatibility and to possibly simplify the port specification.

   .. note:: Make sure the ranges are sufficiently large for security. A
      desirable size depends on several parameters, but we generally recommend
      it contain at least 16384 ports (14 bits of entropy). Note also that the
      system's default range when used may be too small for this purpose, and
      that the range may even be changed while :iscman:`named` is running; the new
      range is automatically applied when :iscman:`named` is reloaded. Explicit
      configuration of :any:`use-v4-udp-ports` and :any:`use-v6-udp-ports` is encouraged,
      so that the ranges are sufficiently large and are reasonably
      independent from the ranges used by other applications.

   .. note:: The operational configuration where :iscman:`named` runs may prohibit
      the use of some ports. For example, Unix systems do not allow
      :iscman:`named`, if run without root privilege, to use ports less than 1024.
      If such ports are included in the specified (or detected) set of query
      ports, the corresponding query attempts will fail, resulting in
      resolution failures or delay. It is therefore important to configure the
      set of ports that can be safely used in the expected operational
      environment.

   .. warning:: Specifying a single port is discouraged, as it removes a layer of
      protection against spoofing errors.

   .. warning:: The configured :term:`port` must not be the same as the listening port.

   .. note:: See also :any:`transfer-source`, :any:`notify-source` and :any:`parental-source`.

.. _zone_transfers:

Zone Transfers
^^^^^^^^^^^^^^

BIND has mechanisms in place to facilitate zone transfers and set limits
on the amount of load that transfers place on the system. The following
options apply to zone transfers.

.. namedconf:statement:: also-notify
   :tags: transfer
   :short: Defines one or more hosts that are sent ``NOTIFY`` messages when zone changes occur.

   This option defines a global list of IP addresses of name servers that are also
   sent NOTIFY messages whenever a fresh copy of the zone is loaded, in
   addition to the servers listed in the zone's NS records. This helps
   to ensure that copies of the zones quickly converge on stealth
   servers. Optionally, a port may be specified with each
   :any:`also-notify` address to send the notify messages to a port other
   than the default of 53. An optional TSIG key can also be specified
   with each address to cause the notify messages to be signed; this can
   be useful when sending notifies to multiple views. In place of
   explicit addresses, one or more named :any:`primaries` lists can be used.

   If an :any:`also-notify` list is given in a :any:`zone` statement, it
   overrides the ``options also-notify`` statement. When a
   ``zone notify`` statement is set to ``no``, the IP addresses in the
   global :any:`also-notify` list are not sent NOTIFY messages for that
   zone. The default is the empty list (no global notification list).

.. namedconf:statement:: min-transfer-rate-in
   :tags: transfer
   :short: Specifies the minimum traffic rate below which inbound zone transfers are terminated.

   Inbound zone transfers running slower than the given amount of bytes in the
   given amount of minutes are terminated. This option takes two non-zero integer values.
   A check is performed periodically every time the configured time interval
   passes. The default value is ``10240 5``, i.e. 10240 bytes in 5 minutes.
   The maximum time value is 28 days (40320 minutes).

.. namedconf:statement:: max-transfer-time-in
   :tags: transfer
   :short: Specifies the number of minutes after which inbound zone transfers are terminated.

   Inbound zone transfers running longer than this many minutes are
   terminated. The default is 120 minutes (2 hours). The maximum value
   is 28 days (40320 minutes).

.. namedconf:statement:: max-transfer-idle-in
   :tags: transfer
   :short: Specifies the number of minutes after which inbound zone transfers making no progress are terminated.

   Inbound zone transfers making no progress in this many minutes are
   terminated. The default is 60 minutes (1 hour). The maximum value
   is 28 days (40320 minutes).

   .. note:: Inbound zone transfers are also affected by
             ``tcp-idle-timeout``; ``max-transfer-idle-in`` closes the
             inbound zone transfer if there is no complete AXFR or no complete
             IXFR chunk. ``tcp-idle-timeout`` closes the connection if
             there is no progress on the TCP level.

.. namedconf:statement:: max-transfer-time-out
   :tags: transfer
   :short: Specifies the number of minutes after which outbound zone transfers are terminated.

   Outbound zone transfers running longer than this many minutes are
   terminated. The default is 120 minutes (2 hours). The maximum value
   is 28 days (40320 minutes).

.. namedconf:statement:: max-transfer-idle-out
   :tags: transfer
   :short: Specifies the number of minutes after which outbound zone transfers making no progress are terminated.

   Outbound zone transfers making no progress in this many minutes are
   terminated. The default is 60 minutes (1 hour). The maximum value
   is 28 days (40320 minutes).

.. namedconf:statement:: notify-rate
   :tags: transfer, zone
   :short: Specifies the rate at which NOTIFY requests are sent during normal zone maintenance operations.

   This specifies the rate at which NOTIFY requests are sent during normal zone
   maintenance operations. (NOTIFY requests due to initial zone loading
   are subject to a separate rate limit; see below.) The default is 20
   per second. The lowest possible rate is one per second; when set to
   zero, it is silently raised to one.

.. namedconf:statement:: primaries
   :tags: transfer, zone
   :short: Defines one or more servers that zone transfer can be requested from.

   This specifies a list of one or more IP addresses of primary servers that
   the secondary contacts to update its copy of the zone. Primaries list
   elements can also be names of :any:`remote-servers` blocks.

   By default, transfers are made from port 53 on the servers; this can be
   changed for all servers by specifying a port number before the list of IP
   addresses, or on a per-server basis after the IP address. Authentication to
   the primary can also be done with per-server TSIG keys.

.. namedconf:statement:: startup-notify-rate
   :tags: transfer, zone
   :short: Specifies the rate at which NOTIFY requests are sent when the name server is first starting, or when new zones have been added.

   This is the rate at which NOTIFY requests are sent when the name server
   is first starting up, or when zones have been newly added to the
   name server. The default is 20 per second. The lowest possible rate is
   one per second; when set to zero, it is silently raised to one.

.. namedconf:statement:: serial-query-rate
   :tags: transfer
   :short: Defines an upper limit on the number of queries per second issued by the server, when querying the SOA RRs used for zone transfers.

   Secondary servers periodically query primary servers to find out if
   zone serial numbers have changed. Each such query uses a minute
   amount of the secondary server's network bandwidth. To limit the amount
   of bandwidth used, BIND 9 limits the rate at which queries are sent.
   The value of the :any:`serial-query-rate` option, an integer, is the
   maximum number of queries sent per second. The default is 20 per
   second. The lowest possible rate is one per second; when set to zero,
   it is silently raised to one.

.. namedconf:statement:: transfer-format
   :tags: transfer
   :short: Controls whether multiple records can be packed into a message during zone transfers.

   Zone transfers can be sent using two different formats,
   ``one-answer`` and ``many-answers``. The :any:`transfer-format` option
   is used on the primary server to determine which format it sends.
   ``one-answer`` uses one DNS message per resource record transferred.
   ``many-answers`` packs as many resource records as possible into one
   message. ``many-answers`` is more efficient; the default is ``many-answers``.
   :any:`transfer-format` may be overridden on a per-server basis by using
   the :namedconf:ref:`server` block.

.. namedconf:statement:: transfer-message-size
   :tags: transfer
   :short: Limits the uncompressed size of DNS messages used in zone transfers over TCP.

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

.. namedconf:statement:: transfers-in
   :tags: transfer
   :short: Limits the number of concurrent inbound zone transfers.

   This is the maximum number of inbound zone transfers that can run
   concurrently. The default value is ``10``. Increasing
   :any:`transfers-in` may speed up the convergence of secondary zones, but it
   also may increase the load on the local system.

.. namedconf:statement:: transfers-out
   :tags: transfer
   :short: Limits the number of concurrent outbound zone transfers.

   This is the maximum number of outbound zone transfers that can run
   concurrently. Zone transfer requests in excess of the limit are
   refused. The default value is ``10``.

.. namedconf:statement:: transfers-per-ns
   :tags: transfer
   :short: Limits the number of concurrent inbound zone transfers from a remote server.

   This is the maximum number of inbound zone transfers that can concurrently
   transfer from a given remote name server. The default value is
   ``2``. Increasing :any:`transfers-per-ns` may speed up the convergence
   of secondary zones, but it also may increase the load on the remote name
   server. :any:`transfers-per-ns` may be overridden on a per-server basis
   by using the :any:`transfers` phrase of the :namedconf:ref:`server` statement.

.. namedconf:statement:: transfer-source
   :tags: transfer
   :short: Defines which local IPv4 address(es) are bound to TCP connections used to fetch zones transferred inbound by the server.

   :any:`transfer-source` determines which local address is bound to
   IPv4 TCP connections used to fetch zones transferred inbound by the
   server. It also determines the source IPv4 address, and optionally
   the UDP port, used for the refresh queries and forwarded dynamic
   updates. If not set, it defaults to a system-controlled value which
   is usually the address of the interface "closest to" the remote
   end. This address must appear in the remote end's :any:`allow-transfer`
   option for the zone being transferred, if one is specified. This
   statement sets the :any:`transfer-source` for all zones, but can be
   overridden on a per-view or per-zone basis by including a
   :any:`transfer-source` statement within the :any:`view` or :any:`zone` block
   in the configuration file.

   .. note:: ``port`` configuration is deprecated. A warning will be logged
      when this parameter is used.

   .. warning:: Specifying a single port is discouraged, as it removes a layer of
      protection against spoofing errors.

   .. warning:: The configured :term:`port` must not be the same as the listening port.

.. namedconf:statement:: transfer-source-v6
   :tags: transfer
   :short: Defines which local IPv6 address(es) are bound to TCP connections used to fetch zones transferred inbound by the server.

   This option is the same as :any:`transfer-source`, except zone transfers
   are performed using IPv6.

.. namedconf:statement:: notify-source
   :tags: transfer
   :short: Defines the IPv4 address (and optional port) to be used for outgoing ``NOTIFY`` messages.

   :any:`notify-source` determines which local source address, and
   optionally UDP port, is used to send NOTIFY messages. This
   address must appear in the secondary server's :any:`primaries` zone clause or
   in an :any:`allow-notify` clause. This statement sets the
   :any:`notify-source` for all zones, but can be overridden on a per-zone
   or per-view basis by including a :any:`notify-source` statement within
   the :any:`zone` or :any:`view` block in the configuration file.

   .. note:: ``port`` configuration is deprecated. A warning will be logged
      when this parameter is used.

   .. warning:: Specifying a single port is discouraged, as it removes a layer of
      protection against spoofing errors.

   .. warning:: The configured :term:`port` must not be the same as the listening port.

.. namedconf:statement:: notify-source-v6
   :tags: transfer
   :short: Defines the IPv6 address (and optional port) to be used for outgoing ``NOTIFY`` messages.

   This option acts like :any:`notify-source`, but applies to ``NOTIFY`` messages sent to IPv6
   addresses.

.. _server_resource_limits:

Server Resource Limits
^^^^^^^^^^^^^^^^^^^^^^

The following options set limits on the server's resource consumption
that are enforced internally by the server rather than by the operating
system.

.. namedconf:statement:: max-journal-size
   :tags: transfer
   :short: Controls the size of journal files.

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

.. namedconf:statement:: max-records
   :tags: zone, server
   :short: Sets the maximum number of records permitted in a zone.

   This sets the maximum number of records permitted in a zone. The default is
   zero, which means the maximum is unlimited.

.. namedconf:statement:: max-records-per-type
   :tags: server
   :short: Sets the maximum number of records that can be stored in an RRset.

   This sets the maximum number of resource records that can be stored
   in an RRset in a database. When configured in :namedconf:ref:`options`
   or :namedconf:ref:`view`, it controls the cache database; it also sets
   the default value for zone databases, which can be overridden by setting
   it at the :namedconf:ref:`zone` level.

   If set to a positive value, any attempt to cache, or to add to a zone
   an RRset with more than the specified number of records, will result in
   a failure. If set to 0, there is no cap on RRset size. The default is
   100.

.. namedconf:statement:: max-types-per-name
   :tags: server
   :short: Sets the maximum number of RR types that can be stored for an owner name.

   This sets the maximum number of resource record types that can be stored
   for a single owner name in a database. When configured in
   :namedconf:ref:`options` or :namedconf:ref:`view`, it controls the cache
   database and sets the default value for zone databases, which can be
   overridden by setting it at the :namedconf:ref:`zone` level.

   An RR type and its corresponding signature are counted as two types. So,
   for example, a signed node containing A and AAAA records has four types:
   A, RRSIG(A), AAAA, and RRSIG(AAAA).

   The behavior is slightly different for zone and cache databases:

   In a zone, if :any:`max-types-per-name` is set to a positive number, any
   attempt to add a new resource record set to a name that already has the
   specified number of types will fail.

   In a cache, if :any:`max-types-per-name` is set to a positive number, an
   attempt to add a new resource record set to a name that already has the
   specified number of types will temporarily succeed, so that the query can
   be answered. However, the newly added RRset will immediately be purged.

   Certain high-priority types, including SOA, CNAME, DNSKEY, and their
   corresponding signatures, are always cached. If :any:`max-types-per-name`
   is set to a very low value, then it may be ignored to allow high-priority
   types to be cached.

   When :any:`max-types-per-name` is set to 0, there is no cap on the number
   of RR types.  The default is 100.

.. namedconf:statement:: recursive-clients
   :tags: query
   :short: Specifies the maximum number of concurrent recursive queries the server can perform.

   This sets the maximum number (a "hard quota") of simultaneous recursive lookups
   the server performs on behalf of clients. The default is
   ``1000``. Because each recursing client uses a fair bit of memory (on
   the order of 20 kilobytes), the value of the :any:`recursive-clients`
   option may have to be decreased on hosts with limited memory.

   :any:`recursive-clients` defines a "hard quota" limit for pending
   recursive clients; when more clients than this are pending, new
   incoming requests are not accepted, and for each incoming request
   a previous pending request is dropped.

   A "soft quota" is also set. When this lower quota is exceeded,
   incoming requests are accepted, but for each one, a pending request
   is dropped. If :any:`recursive-clients` is greater than 1000, the
   soft quota is set to :any:`recursive-clients` minus 100; otherwise it is
   set to 90% of :any:`recursive-clients`.

.. namedconf:statement:: tcp-clients
   :tags: server
   :short: Specifies the maximum number of simultaneous client TCP connections accepted by the server.

   This is the maximum number of simultaneous client TCP connections that the
   server accepts. The default is ``150``.

.. namedconf:statement:: clients-per-query
   :tags: server
   :short: Sets the initial minimum number of simultaneous recursive clients accepted by the server for any given query before the server drops additional clients.

   This sets the initial value (minimum) number of simultaneous recursive clients
   for any given query (<qname,qtype,qclass>) that the server accepts before
   dropping additional clents. :iscman:`named` attempts to self-tune this
   value and changes are logged. The default value is 10.

   The chosen value should reflect how many queries come in for a given name
   in the time it takes to resolve that name.

.. namedconf:statement:: max-clients-per-query
   :tags: server
   :short: Sets the maximum number of simultaneous recursive clients accepted by the server for any given query before the server drops additional clients.

   This sets the maximum number of simultaneous recursive clients for any
   given query (<qname,qtype,qclass>) that the server accepts before
   dropping additional clients.

   If the number of queries exceeds :any:`clients-per-query`, :iscman:`named`
   assumes that it is dealing with a non-responsive zone and drops additional
   queries. If it gets a response after dropping queries, it raises the estimate,
   up to a limit of :any:`max-clients-per-query`. The estimate is then lowered
   after 20 minutes if it has remained unchanged.

   If :any:`max-clients-per-query` is set to zero, there is no upper bound, other
   than that imposed by :any:`recursive-clients`. If the option is set to a
   lower value than :any:`clients-per-query`, the value is adjusted to
   :any:`clients-per-query`.

   If :any:`clients-per-query` is set to zero, :any:`max-clients-per-query` no
   longer applies and there is no upper bound, other than that imposed by
   :any:`recursive-clients`.

.. namedconf:statement:: max-validations-per-fetch
   :tags: server
   :short: Sets the maximum number of DNSSEC validations that can happen in a single fetch.

   This is an **experimental** setting that defines the maximum number of DNSSEC
   validations that can happen in a single resolver fetch. The default is 16.

.. namedconf:statement:: max-validation-failures-per-fetch
   :tags: server
   :short: Sets the maximum number of DNSSEC validation failures that can happen in a single fetch.

   This is an **experimental** setting that defines the maximum number of DNSSEC
   validation failures that can happen in a single resolver fetch. The default
   is 1.

.. namedconf:statement:: fetches-per-zone
   :tags: server, query
   :short: Sets the maximum number of simultaneous iterative queries allowed to any one domain before the server blocks new queries for data in or beneath that zone.

   This sets the maximum number of simultaneous iterative queries to any one
   domain that the server permits before blocking new queries for
   data in or beneath that zone. This value should reflect how many
   fetches would normally be sent to any one zone in the time it would
   take to resolve them. It should be smaller than
   :any:`recursive-clients`.

   When many clients simultaneously query for the same name and type,
   the clients are all attached to the same fetch, up to the
   :any:`max-clients-per-query` limit, and only one iterative query is
   sent. However, when clients are simultaneously querying for
   *different* names or types, multiple queries are sent and
   :any:`max-clients-per-query` is not effective as a limit.

   Optionally, this value may be followed by the keyword ``drop`` or
   ``fail``, indicating whether queries which exceed the fetch quota for
   a zone are dropped with no response, or answered with SERVFAIL.
   The default is ``drop``.

   If :any:`fetches-per-zone` is set to zero, there is no limit on the
   number of fetches per query and no queries are dropped. The
   default is zero.

   The current list of active fetches can be dumped by running
   :option:`rndc recursing`. The list includes the number of active fetches
   for each domain and the number of queries that have been passed
   (allowed) or dropped (spilled) as a result of the :any:`fetches-per-zone`
   limit. (Note: these counters are not cumulative over time;
   whenever the number of active fetches for a domain drops to zero,
   the counter for that domain is deleted, and the next time a fetch
   is sent to that domain, it is recreated with the counters set
   to zero.)

   .. note::

       Fetches generated automatically in the result of :any:`prefetch` are
       exempt from this quota.

.. namedconf:statement:: fetches-per-server
   :tags: server, query
   :short: Sets the maximum number of simultaneous iterative queries allowed to be sent by a server to an upstream name server before the server blocks additional queries.

   This sets the maximum number of simultaneous iterative queries that the server
   allows to be sent to a single upstream name server before
   blocking additional queries. This value should reflect how many
   fetches would normally be sent to any one server in the time it would
   take to resolve them. It should be smaller than
   :any:`recursive-clients`.

   Optionally, this value may be followed by the keyword ``drop`` or
   ``fail``, indicating whether queries are dropped with no
   response or answered with SERVFAIL, when all of the servers
   authoritative for a zone are found to have exceeded the per-server
   quota. The default is ``fail``.

   If :any:`fetches-per-server` is set to zero, there is no limit on
   the number of fetches per query and no queries are dropped. The
   default is zero.

   The :any:`fetches-per-server` quota is dynamically adjusted in response
   to detected congestion. As queries are sent to a server and either are
   answered or time out, an exponentially weighted moving average
   is calculated of the ratio of timeouts to responses. If the current
   average timeout ratio rises above a "high" threshold, then
   :any:`fetches-per-server` is reduced for that server. If the timeout
   ratio drops below a "low" threshold, then :any:`fetches-per-server` is
   increased. The :any:`fetch-quota-params` options can be used to adjust
   the parameters for this calculation.

   .. note::

       Fetches generated automatically in the result of :any:`prefetch` are
       exempt from this quota, but they are included in the quota calculations.

.. namedconf:statement:: fetch-quota-params
   :tags: server, query
   :short: Sets the parameters for dynamic resizing of the :any:`fetches-per-server` quota in response to detected congestion.

   This sets the parameters to use for dynamic resizing of the
   :any:`fetches-per-server` quota in response to detected congestion.

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

.. namedconf:statement:: max-cache-size
   :tags: server
   :short: Sets the maximum amount of memory to use for an individual cache database and its associated metadata.

   This sets the maximum amount of memory to use for an individual cache
   database and its associated metadata, in bytes or percentage of total
   physical memory. By default, each view has its own separate cache,
   which means the total amount of memory required for cache data is the
   sum of the cache database sizes for all views (unless the
   :any:`attach-cache` option is used).

   When the amount of data in a cache database reaches the configured
   limit, :iscman:`named` starts purging non-expired records (following an
   LRU-based strategy).

   The default size limit for each individual cache is:

     - 90% of physical memory for views with :any:`recursion` set to
       ``yes`` (the default), or

     - 2 MB for views with :any:`recursion` set to ``no``.

   Any positive value smaller than 2 MB is ignored and reset to 2 MB.
   The keyword ``unlimited``, or the value ``0``, places no limit on the
   cache size; records are then purged from the cache only when they
   expire (according to their TTLs).

   .. note::

       For configurations which define multiple views with separate
       caches and recursion enabled, it is recommended to set
       :any:`max-cache-size` appropriately for each view, as using the
       default value of that option (90% of physical memory for each
       individual cache) may lead to memory exhaustion over time.

   .. note::

       :any:`max-cache-size` does not work reliably for a maximum
       amount of memory of 100 MB or lower.

   Upon startup and reconfiguration, caches with a limited size
   preallocate a small amount of memory (less than 1% of
   :any:`max-cache-size` for a given view). This preallocation serves as an
   optimization to eliminate extra latency introduced by resizing
   internal cache structures.

   On systems where detection of the amount of physical memory is not
   supported, percentage-based values fall back to ``unlimited``. Note
   that the amount of physical memory available is only detected on
   startup, so :iscman:`named` does not adjust the cache size limits if the
   amount of physical memory is changed at runtime.

.. namedconf:statement:: tcp-listen-queue
   :tags: server
   :short: Sets the listen-queue depth.

   This sets the listen-queue depth. The default and minimum is 10. If the kernel
   supports the accept filter "dataready", this also controls how many
   TCP connections are queued in kernel space waiting for some
   data before being passed to accept. Non-zero values less than 10 are
   silently raised. A value of 0 may also be used; on most platforms
   this sets the listen-queue length to a system-defined default value.

.. namedconf:statement:: tcp-initial-timeout
   :tags: server, query
   :short: Sets the amount of time (in milliseconds) that the server waits on a new TCP connection for the first message from the client.

   This sets the amount of time, in units of 100 milliseconds, that the server waits on
   a new TCP connection for the first message from the client. The
   default is 300 (30 seconds), the minimum is 25 (2.5 seconds), and the
   maximum is 1200 (two minutes). Values above the maximum or below the
   minimum are adjusted with a logged warning. (Note: this value
   must be greater than the expected round-trip delay time; otherwise, no
   client will ever have enough time to submit a message.) This value
   can be updated at runtime by using :option:`rndc tcp-timeouts`.

.. namedconf:statement:: tcp-idle-timeout
   :tags: query
   :short: Sets the amount of time (in milliseconds) that the server waits on an idle TCP connection before closing it, if the EDNS TCP keepalive option is not in use.

   This sets the amount of time, in units of 100 milliseconds, that the server waits on
   an idle TCP connection before closing it, when the client is not using
   the EDNS TCP keepalive option. The default is 300 (30 seconds), the
   maximum is 1200 (two minutes), and the minimum is 1 (one-tenth of a
   second). Values above the maximum or below the minimum are
   adjusted with a logged warning. See :any:`tcp-keepalive-timeout` for
   clients using the EDNS TCP keepalive option. This value can be
   updated at runtime by using :option:`rndc tcp-timeouts`.

.. namedconf:statement:: tcp-keepalive-timeout
   :tags: query
   :short: Sets the amount of time (in milliseconds) that the server waits on an idle TCP connection before closing it, if the EDNS TCP keepalive option is in use.

   This sets the amount of time, in units of 100 milliseconds, that the server waits on
   an idle TCP connection before closing it, when the client is using the
   EDNS TCP keepalive option. The default is 300 (30 seconds), the
   maximum is 65535 (about 1.8 hours), and the minimum is 1 (one-tenth
   of a second). Values above the maximum or below the minimum are
   adjusted with a logged warning. This value may be greater than
   :any:`tcp-idle-timeout` because clients using the EDNS TCP keepalive
   option are expected to use TCP connections for more than one message.
   This value can be updated at runtime by using :option:`rndc tcp-timeouts`.

.. namedconf:statement:: tcp-advertised-timeout
   :tags: query
   :short: Sets the timeout value (in milliseconds) that the server sends in responses containing the EDNS TCP keepalive option.

   This sets the timeout value, in units of 100 milliseconds, that the server sends
   in responses containing the EDNS TCP keepalive option, which informs a
   client of the amount of time it may keep the session open. The
   default is 300 (30 seconds), the maximum is 65535 (about 1.8 hours),
   and the minimum is 0, which signals that the clients must close TCP
   connections immediately. Ordinarily this should be set to the same
   value as :any:`tcp-keepalive-timeout`. This value can be updated at
   runtime by using :option:`rndc tcp-timeouts`.

.. namedconf:statement:: update-quota
   :tags: server
   :short: Specifies the maximum number of concurrent DNS UPDATE messages that can be processed by the server.

   This is the maximum number of simultaneous DNS UPDATE messages that
   the server will accept, for updating local authoritative zones or
   forwarding to a primary server. The default is ``100``.

.. namedconf:statement:: sig0checks-quota
   :tags: server
   :short: Specifies the maximum number of concurrent SIG(0) signature checks that can be processed by the server.

   This is the maximum number of simultaneous SIG(0)-signed messages that
   the server accepts. If the quota is reached, then :iscman:`named` answers
   with a status code of REFUSED. The value of ``0`` disables the quota. The
   default is ``1``.

.. namedconf:statement:: sig0checks-quota-exempt
   :tags: server
   :short: Exempts specific clients or client groups from SIG(0) signature checking quota.

   DNS clients can be exempted from the SIG(0) signature checking quota with the
   :any:`sig0checks-quota-exempt` clause, using their IP and/or network
   addresses. The default value is an empty list.

   Example:

   ::

       sig0checks-quota-exempt {
           10.0.0.0/8;
           2001:db8::100;
       };

.. namedconf:statement:: sig0key-checks-limit
   :tags: server
   :short: Specifies the maximum number of SIG(0) keys to consider when trying to verify a message.

   This is the maximum number of keys to consider for a SIG(0)-signed message
   when trying to verify it. :iscman:`named` will parse the candidate keys and
   check whether their key tag and algorithm matches with the expected one
   before trying to verify the signature. If the limit is reached the message
   verification fails. The value of ``0`` disables the limitation. The default
   is ``16``.

.. namedconf:statement:: sig0message-checks-limit
   :tags: server
   :short: Specifies the maximum number of matching SIG(0) keys to try to verify a message.

   This is the maximum number of keys which (when correctly parsed and matched
   against the expected key tag and algorithm) :iscman:`named` uses to verify
   a SIG(0)-signed message. If the limit is reached the message verification
   fails. The value of ``0`` disables the limitation. The default is ``2``.

.. _intervals:

Periodic Task Intervals
^^^^^^^^^^^^^^^^^^^^^^^

.. namedconf:statement:: heartbeat-interval
   :tags: deprecated
   :short: Sets the interval at which the server performs zone maintenance tasks for all zones marked as :any:`dialup`.

   The server performs zone maintenance tasks for all zones marked
   as :any:`dialup` whenever this interval expires. The default is 60
   minutes. Reasonable values are up to 1 day (1440 minutes). The
   maximum value is 28 days (40320 minutes). If set to 0, no zone
   maintenance for these zones occurs.

   This option is deprecated and will be removed in a future release.

.. namedconf:statement:: interface-interval
   :tags: server
   :short: Sets the interval at which the server scans the network interface list.

   The server scans the network interface list on every interval as specified by
   :any:`interface-interval`.

   If set to 0, interface scanning only occurs when the configuration
   file is loaded, or when :any:`automatic-interface-scan` is enabled and supported
   by the operating system. After the scan, the server begins listening for
   queries on any newly discovered interfaces (provided they are allowed by the
   :any:`listen-on` configuration), and stops listening on interfaces that have
   gone away. For convenience, TTL-style time-unit suffixes may be used to
   specify the value. It also accepts ISO 8601 duration formats.

   The default is 60 minutes (1 hour); the maximum value is 28 days.

The :any:`sortlist` Statement
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The response to a DNS query may consist of multiple resource records
(RRs) forming a resource record set (RRset). The name server
normally returns the RRs within the RRset in an indeterminate order (but
see the :any:`rrset-order` statement in :ref:`rrset_ordering`). The client resolver code should
rearrange the RRs as appropriate: that is, using any addresses on the
local net in preference to other addresses. However, not all resolvers
can do this or are correctly configured. When a client is using a local
server, the sorting can be performed in the server, based on the
client's address. This only requires configuring the name servers, not
all the clients.

.. namedconf:statement:: sortlist
   :tags: query, deprecated
   :short: Controls the ordering of RRs returned to the client, based on the client's IP address.

   This option is deprecated and will be removed in a future release.

   The :any:`sortlist` statement (see below) takes an :term:`address_match_list` and
   interprets it in a special way. Each top-level statement in the :any:`sortlist`
   must itself be an explicit :term:`address_match_list` with one or two elements. The
   first element (which may be an IP address, an IP prefix, an ACL name, or a nested
   :term:`address_match_list`) of each top-level list is checked against the source
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

The following example illustrates reasonable behavior for the local host
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

.. namedconf:statement:: rrset-order
   :tags: query
   :short: Defines the order in which equal RRs (RRsets) are returned.

   The :any:`rrset-order` statement permits configuration of the ordering of
   the records in a multiple-record response. See also:
   :any:`sortlist`.

   Each rule in an :any:`rrset-order` statement is defined as follows:

   ::

       [class <class_name>] [type <type_name>] [name "<domain_name>"] order <ordering>

   The default qualifiers for each rule are:

     - If no ``class`` is specified, the default is ``ANY``.
     - If no :any:`type` is specified, the default is ``ANY``.
     - If no ``name`` is specified, the default is ``*`` (asterisk).

   :term:`<domain_name> <domain_name>` only matches the name itself, not any of its
   subdomains.  To make a rule match all subdomains of a given name, a
   wildcard name (``*.<domain_name>``) must be used.  Note that
   ``*.<domain_name>`` does *not* match ``<domain_name>`` itself; to
   specify RRset ordering for a name and all of its subdomains, two
   separate rules must be defined: one for ``<domain_name>`` and one for
   ``*.<domain_name>``.

   The legal values for ``<ordering>`` are:

   ``fixed``
       Records are returned in the order they are defined in the zone file.

       This value is deprecated and will be removed in a future release.

   .. note::

       The ``fixed`` option is only available if BIND is configured with
       ``--enable-fixed-rrset`` at compile time.

   ``random``
       Records are returned in a non-deterministic order.  The random ordering
       doesn't guarantee uniform distribution of all permutations.

   ``cyclic``
       Records are returned in a cyclic round-robin order, rotating by one
       record per query.

   ``none``
       Records are returned in the order they were retrieved from the
       database. This order is indeterminate, but remains consistent as
       long as the database is not modified.

   The default RRset order used depends on whether any :any:`rrset-order`
   statements are present in the configuration file used by :iscman:`named`:

     - If no :any:`rrset-order` statement is present in the configuration
       file, the implicit default is to return all records in ``random``
       order.

     - If any :any:`rrset-order` statements are present in the configuration
       file, but no ordering rule specified in these statements matches a
       given RRset, the default order for that RRset is ``none``.

   Note that if multiple :any:`rrset-order` statements are present in the
   configuration file (at both the :namedconf:ref:`options` and :any:`view` levels), they
   are *not* combined; instead, the more-specific one (:any:`view`) replaces
   the less-specific one (:namedconf:ref:`options`).

   If multiple rules within a single :any:`rrset-order` statement match a
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

.. namedconf:statement:: lame-ttl
   :tags: server
   :short: Sets the resolver's lame cache.

   This is always set to 0. More information is available in the
   `security advisory for CVE-2021-25219
   <https://kb.isc.org/docs/cve-2021-25219>`_.

.. namedconf:statement:: servfail-ttl
   :tags: server
   :short: Sets the length of time (in seconds) that a SERVFAIL response is cached.

   This sets the number of seconds to cache a SERVFAIL response due to DNSSEC
   validation failure or other general server failure. If set to ``0``,
   SERVFAIL caching is disabled. The SERVFAIL cache is not consulted if
   a query has the CD (Checking Disabled) bit set; this allows a query
   that failed due to DNSSEC validation to be retried without waiting
   for the SERVFAIL TTL to expire.

   The maximum value is ``30`` seconds; any higher value is
   silently reduced. The default is ``1`` second.

.. namedconf:statement:: min-ncache-ttl
   :tags: server
   :short: Specifies the minimum retention time (in seconds) for storage of negative answers in the server's cache.

   To reduce network traffic and increase performance, the server stores
   negative answers. :any:`min-ncache-ttl` is used to set a minimum
   retention time for these answers in the server, in seconds. For
   convenience, TTL-style time-unit suffixes may be used to specify the
   value. It also accepts ISO 8601 duration formats.

   The default :any:`min-ncache-ttl` is ``0`` seconds. :any:`min-ncache-ttl` cannot
   exceed 90 seconds and is truncated to 90 seconds if set to a greater
   value.

.. namedconf:statement:: min-cache-ttl
   :tags: server
   :short: Specifies the minimum time (in seconds) that the server caches ordinary (positive) answers.

   This sets the minimum time for which the server caches ordinary (positive)
   answers, in seconds. For convenience, TTL-style time-unit suffixes may be used
   to specify the value. It also accepts ISO 8601 duration formats.

   The default :any:`min-cache-ttl` is ``0`` seconds. :any:`min-cache-ttl` cannot
   exceed 90 seconds and is truncated to 90 seconds if set to a greater
   value.

.. namedconf:statement:: max-ncache-ttl
   :tags: server
   :short: Specifies the maximum retention time (in seconds) for storage of negative answers in the server's cache.

   To reduce network traffic and increase performance, the server stores
   negative answers. :any:`max-ncache-ttl` is used to set a maximum retention time
   for these answers in the server, in seconds. For convenience, TTL-style
   time-unit suffixes may be used to specify the value.  It also accepts ISO 8601
   duration formats.

   The default :any:`max-ncache-ttl` is 10800 seconds (3 hours). :any:`max-ncache-ttl`
   cannot exceed 7 days and is silently truncated to 7 days if set to a
   greater value.

.. namedconf:statement:: max-cache-ttl
   :tags: server
   :short: Specifies the maximum time (in seconds) that the server caches ordinary (positive) answers.

   This sets the maximum time for which the server caches ordinary (positive)
   answers, in seconds. For convenience, TTL-style time-unit suffixes may be used
   to specify the value. It also accepts ISO 8601 duration formats.

   The default :any:`max-cache-ttl` is 604800 (one week). A value of zero may cause
   all queries to return SERVFAIL, because of lost caches of intermediate RRsets
   (such as NS and glue AAAA/A records) in the resolution process.

.. namedconf:statement:: max-stale-ttl
   :tags: server
   :short: Specifies the maximum time that the server retains records past their normal expiry, to return them as stale records.

   If retaining stale RRsets in cache is enabled, and returning of stale cached
   answers is also enabled, :any:`max-stale-ttl` sets the maximum time for which
   the server retains records past their normal expiry to return them as stale
   records, when the servers for those records are not reachable. The default
   is 1 day. The minimum allowed is 1 second; a value of 0 is updated silently
   to 1 second.

   For stale answers to be returned, the retaining of them in cache must be
   enabled via the configuration option :any:`stale-cache-enable`, and returning
   cached answers must be enabled, either in the configuration file using the
   :any:`stale-answer-enable` option or by calling :option:`rndc serve-stale on <rndc serve-stale>`.

   When :any:`stale-cache-enable` is set to ``no``, setting the :any:`max-stale-ttl`
   has no effect; the value of :any:`max-stale-ttl` is ``0`` in such a case.

.. namedconf:statement:: sig-validity-interval
   :tags: obsolete

   This option no longer has any effect.

.. namedconf:statement:: dnskey-sig-validity
   :tags: obsolete

   This option no longer has any effect.

.. namedconf:statement:: sig-signing-nodes
   :tags: dnssec
   :short: Specifies the maximum number of nodes to be examined in each quantum, when signing a zone with a new DNSKEY.

   The default is ``100``.

.. namedconf:statement:: sig-signing-signatures
   :tags: dnssec
   :short: Specifies the threshold for the number of signatures that terminates processing a quantum, when signing a zone with a new DNSKEY.

   The default is ``10``.

.. namedconf:statement:: sig-signing-type
   :tags: dnssec
   :short: Specifies a private RDATA type to use when generating signing-state records.

   The default is ``65534``.

   This parameter may be removed in a future version, once there is a standard
   type.

   Signing-state records are used internally by :iscman:`named` to track
   the current state of a zone-signing process, i.e., whether it is
   still active or has been completed. The records can be inspected
   using the command :option:`rndc signing -list zone <rndc signing>`.
   Once :iscman:`named` has finished signing a zone with a particular key,
   the signing-state record associated with that key can be removed from the
   zone by running
   :option:`rndc signing -clear keyid/algorithm zone <rndc signing>`.
   To clear all of the completed signing-state records for a zone, use
   :option:`rndc signing -clear all zone <rndc signing>`.

.. namedconf:statement:: min-refresh-time
   :tags: transfer
   :short: Limits the zone refresh interval to no more often than the specified value, in seconds.

   This option controls the server's behavior on refreshing a zone
   (querying for SOA changes). Usually, the SOA refresh values for
   the zone are used; however, these values are set by the primary, giving
   secondary server administrators little control over their contents.

   This option allows the administrator to set a minimum
   refresh time in seconds per-zone, per-view, or globally.
   This option is valid for secondary and stub zones, and clamps the SOA
   refresh time to the specified value.

   The default is 300 seconds.

.. namedconf:statement:: max-refresh-time
   :tags: transfer
   :short: Limits the zone refresh interval to no less often than the specified value, in seconds.

   This option controls the server's behavior on refreshing a zone
   (querying for SOA changes). Usually, the SOA refresh values for
   the zone are used; however, these values are set by the primary, giving
   secondary server administrators little control over their contents.

   This option allows the administrator to set a maximum
   refresh time in seconds per-zone, per-view, or globally.
   This option is valid for secondary and stub zones, and clamps the SOA
   refresh time to the specified value.

   The default is 2419200 seconds (4 weeks).

.. namedconf:statement:: min-retry-time
   :tags: transfer
   :short: Limits the zone refresh retry interval to no more often than the specified value, in seconds.

   This option controls the server's behavior on retrying failed
   zone transfers. Usually, the SOA retry values for the zone are
   used; however, these values are set by the primary, giving
   secondary server administrators little control over their contents.

   This option allows the administrator to set a minimum
   retry time in seconds per-zone, per-view, or globally.
   This option is valid for secondary and stub zones, and clamps the SOA
   retry time to the specified value.

   The default is 500 seconds.

.. namedconf:statement:: max-retry-time
   :tags: transfer
   :short: Limits the zone refresh retry interval to no less often than the specified value, in seconds.

   This option controls the server's behavior on retrying failed
   zone transfers. Usually, the SOA retry values for the zone are
   used; however, these values are set by the primary, giving
   secondary server administrators little control over their contents.

   This option allows the administrator to set a maximum
   retry time in seconds per-zone, per-view, or globally.
   This option is valid for secondary and stub zones, and clamps the SOA
   retry time to the specified value.

   The default is 1209600 seconds (2 weeks).

.. namedconf:statement:: edns-udp-size
   :tags: query
   :short: Sets the maximum advertised EDNS UDP buffer size to control the size of packets received from authoritative servers in response to recursive queries.

   This sets the maximum advertised EDNS UDP buffer size, in bytes, to control
   the size of packets received from authoritative servers in response
   to recursive queries. Valid values are 512 to 4096; values outside
   this range are silently adjusted to the nearest value within it.
   The default value is 1232.

   The usual reason for setting :any:`edns-udp-size` to a non-default value
   is to get UDP answers to pass through broken firewalls that block
   fragmented packets and/or block UDP DNS packets that are greater than
   512 bytes.

   When :iscman:`named` first queries a remote server, it advertises a UDP
   buffer size of 1232.

   Query timeouts observed for any given server affect the buffer size
   advertised in queries sent to that server.  Depending on observed packet
   dropping patterns, the query is retried over TCP.  Per-server EDNS statistics
   are only retained in memory for the lifetime of a given server's ADB entry.

   According to measurements taken by multiple parties, the default value
   should not be causing the fragmentation. As most of the Internet "core" is able to
   cope with IP message sizes between 1400-1500 bytes, the 1232 size was chosen
   as a conservative minimal number that could be changed by the DNS operator to
   a estimated path MTU, minus the estimated header space. In practice, the
   smallest MTU witnessed in the operational DNS community is 1500 octets, the
   Ethernet maximum payload size, so a useful default for the maximum DNS/UDP
   payload size on **reliable** networks would be 1432.

   Any server-specific :any:`edns-udp-size` setting has precedence over all
   the above rules, i.e. configures a static value for a given
   :namedconf:ref:`server` block.

.. namedconf:statement:: max-udp-size
   :tags: query
   :short: Sets the maximum EDNS UDP message size sent by :iscman:`named`.

   This sets the maximum EDNS UDP message size that :iscman:`named` sends, in bytes.
   Valid values are 512 to 4096; values outside this range are
   silently adjusted to the nearest value within it. The default value
   is 1232.

   This value applies to responses sent by a server; to set the
   advertised buffer size in queries, see :any:`edns-udp-size`.

   The usual reason for setting :any:`max-udp-size` to a non-default value
   is to allow UDP answers to pass through broken firewalls that block
   fragmented packets and/or block UDP packets that are greater than 512
   bytes. This is independent of the advertised receive buffer
   (:any:`edns-udp-size`).

   Setting this to a low value encourages additional TCP traffic to
   the name server.

.. namedconf:statement:: masterfile-format
   :tags: zone, server
   :short: Specifies the file format of zone files.

   This specifies the file format of zone files (see :ref:`zonefile_format`
   for details).  The default value is ``text``, which is the standard
   textual representation, except for secondary zones, in which the default
   value is ``raw``. Files in formats other than ``text`` are typically
   expected to be generated by the :iscman:`named-compilezone` tool, or dumped by
   :iscman:`named`.

   Note that when a zone file in a format other than ``text`` is loaded,
   :iscman:`named` may omit some of the checks which are performed for a file in
   ``text`` format. For example, :any:`check-names` only applies when loading
   zones in ``text`` format. Zone files in ``raw`` format should be generated
   with the same check level as that specified in the :iscman:`named`
   configuration file.

   When configured in :namedconf:ref:`options`, this statement sets the
   :any:`masterfile-format` for all zones, but it can be overridden on a
   per-zone or per-view basis by including a :any:`masterfile-format`
   statement within the :any:`zone` or :any:`view` block in the configuration
   file.

.. namedconf:statement:: masterfile-style
   :tags: server
   :short: Specifies the format of zone files during a dump, when the :any:`masterfile-format` is ``text``.

   This specifies the formatting of zone files during dump, when the
   :any:`masterfile-format` is ``text``. This option is ignored with any
   other :any:`masterfile-format`.

   When set to ``relative``, records are printed in a multi-line format,
   with owner names expressed relative to a shared origin. When set to
   ``full``, records are printed in a single-line format with absolute
   owner names. The ``full`` format is most suitable when a zone file
   needs to be processed automatically by a script. The ``relative``
   format is more human-readable, and is thus suitable when a zone is to
   be edited by hand. The default is ``relative``.

.. namedconf:statement:: max-query-count
   :tags: server, query
   :short: Sets the maximum number of iterative queries while servicing a recursive query.

   This sets the maximum number of iterative queries that may be sent
   by a resolver while looking up a single name. If more queries than this
   need to be sent before an answer is reached, then recursion is terminated
   and a SERVFAIL response is returned to the client. The default is ``200``.

.. namedconf:statement:: max-recursion-depth
   :tags: server
   :short: Sets the maximum number of levels of recursion permitted at any one time while servicing a recursive query.

   This sets the maximum number of levels of recursion that are permitted at
   any one time while servicing a recursive query. Resolving a name may
   require looking up a name server address, which in turn requires
   resolving another name, etc.; if the number of recursions exceeds
   this value, the recursive query is terminated and returns SERVFAIL.
   The default is 7.

.. namedconf:statement:: max-recursion-queries
   :tags: server, query
   :short: Sets the maximum number of iterative queries while servicing a recursive query.

   This sets the maximum number of iterative queries that may be sent
   by a resolver while looking up a single name. If more queries than this
   need to be sent before an answer is reached, then recursion is terminated
   and a SERVFAIL response is returned to the client. (Note: if the answer
   is a CNAME, then the subsequent lookup for the target of the CNAME is
   counted separately.) The default is 50.

.. namedconf:statement:: max-query-restarts
   :tags: server, query
   :short: Sets the maximum number of chained CNAMEs to follow

   This sets the maximum number of successive CNAME targets to follow
   when resolving a client query, before terminating the query to avoid a
   CNAME loop. Valid values are 1 to 255. The default is 11.

.. namedconf:statement:: notify-defer
   :tags: transfer, zone
   :short: Sets the defer time (in seconds) before sending NOTIFY messages for a zone.

   This sets the delay, in seconds, to wait before sending a set of NOTIFY
   messages for a zone. Whenever a NOTIFY message is ready to be sent, sending
   will be deferred for this duration. This can be useful, for example, when
   for some operation needs a catalog zone is updated with new member zones
   before these member zones are actually ready to be tranferred. The delay can
   be tuned for the catalog zone to an amount of time after which the member
   zones are usually known to become ready. The default is 0 seconds.

   .. warning::
      This option is not to be confused with the :any:`notify-delay` option.

   .. note::
      An implicit :option:`rndc notify` command for a zone overrides the
      effects of this option.

   .. note::
      This options is ignored for notifies sent during the :any:`dialup`
      process.

.. namedconf:statement:: notify-delay
   :tags: transfer, zone
   :short: Sets the delay (in seconds) between sending sets of NOTIFY messages for a zone.

   This sets the delay, in seconds, between sending sets of NOTIFY messages
   for a zone. Whenever a NOTIFY message is sent for a zone, a timer will
   be set for this duration. If the zone is updated again before the timer
   expires, the NOTIFY for that update will be postponed. The default is 5
   seconds.

   The overall rate at which NOTIFY messages are sent for all zones is
   controlled by :any:`notify-rate`.

   .. warning::
      This option is not to be confused with the :any:`notify-defer` option.

.. namedconf:statement:: max-rsa-exponent-size
   :tags: dnssec, query
   :short: Sets the maximum RSA exponent size (in bits) when validating.

   This sets the maximum RSA exponent size, in bits, that is accepted when
   validating. Valid values are 35 to 4096 bits. The default, zero, is
   also accepted and is equivalent to 4096.

.. namedconf:statement:: prefetch
   :tags: query
   :short: Specifies the "trigger" time-to-live (TTL) value at which prefetch of the current query takes place.

   When a query is received for cached data which is to expire shortly,
   :iscman:`named` can refresh the data from the authoritative server
   immediately, ensuring that the cache always has an answer available.

   :any:`prefetch` specifies the "trigger" TTL value at which prefetch
   of the current query takes place; when a cache record with an
   equal or lower TTL value is encountered during query processing, it is
   refreshed. Valid trigger TTL values are 1 to 10 seconds. Values
   larger than 10 seconds are silently reduced to 10. Setting a
   trigger TTL to zero causes prefetch to be disabled. The default
   trigger TTL is ``2``.

   An optional second argument specifies the "eligibility" TTL: the
   smallest *original* TTL value that is accepted for a record to
   be eligible for prefetching. The eligibility TTL must be at least six
   seconds longer than the trigger TTL; if not, :iscman:`named`
   silently adjusts it upward. The default eligibility TTL is ``9``.

.. namedconf:statement:: v6-bias
   :tags: server, query
   :short: Indicates the number of milliseconds of preference to give to IPv6 name servers.

   When determining the next name server to try, this indicates by how many
   milliseconds to prefer IPv6 name servers. The default is ``50``
   milliseconds.

.. namedconf:statement:: tcp-receive-buffer
   :tags: server
   :short: Sets the operating system's receive buffer size for TCP sockets.

.. namedconf:statement:: udp-receive-buffer
   :tags: server
   :short: Sets the operating system's receive buffer size for UDP sockets.

   These options control the operating system's receive buffer sizes
   (``SO_RCVBUF``) for TCP and UDP sockets, respectively. Buffering at
   the operating system level can prevent packet drops during brief load
   spikes, but if the buffer size is set too high, a running server
   could get clogged with outstanding queries that have already timed
   out. The default is ``0``, which means the operating system's default
   value should be used. The minimum configurable value is ``4096``; any
   nonzero value lower than that is silently raised. The maximum value
   is determined by the kernel, and values exceeding the maximum are
   silently reduced.

.. namedconf:statement:: tcp-send-buffer
   :tags: server
   :short: Sets the operating system's send buffer size for TCP sockets.

.. namedconf:statement:: udp-send-buffer
   :tags: server
   :short: Sets the operating system's send buffer size for UDP sockets.

   These options control the operating system's send buffer sizes
   (``SO_SNDBUF``) for TCP and UDP sockets, respectively. Buffering at
   the operating system level can prevent packet drops during brief load
   spikes, but if the buffer size is set too high, a running server
   could get clogged with outstanding queries that have already timed
   out. The default is ``0``, which means the operating system's default
   value should be used. The minimum configurable value is ``4096``; any
   nonzero value lower than that is silently raised. The maximum value
   is determined by the kernel, and values exceeding the maximum are
   silently reduced.

.. _builtin:

Built-in Server Information Zones
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The server provides some helpful diagnostic information through a number
of built-in zones under the pseudo-top-level-domain ``bind`` in the
``CHAOS`` class. These zones are part of a built-in view
(see :any:`view`) of class ``CHAOS``, which is
separate from the default view of class ``IN``. Most global
configuration options (:any:`allow-query`, etc.) apply to this view,
but some are locally overridden: :namedconf:ref:`notify`, :any:`recursion`, and
:any:`allow-new-zones` are always set to ``no``, and :any:`rate-limit` is set
to allow three responses per second.

To disable these zones, use the options below or hide the
built-in ``CHAOS`` view by defining an explicit view of class ``CHAOS``
that matches all clients.

.. namedconf:statement:: version
   :tags: server
   :short: Specifies the version number of the server to return in response to a ``version.bind`` query.

   This is the version the server should report via a query of the name
   ``version.bind`` with type ``TXT`` and class ``CHAOS``. The default is
   the real version number of this server. Specifying ``version none``
   disables processing of the queries.

   Setting :any:`version` to any value (including ``none``) also disables
   queries for ``authors.bind TXT CH``.

.. namedconf:statement:: hostname
   :tags: server
   :short: Specifies the hostname of the server to return in response to a ``hostname.bind`` query.

   This is the hostname the server should report via a query of the name
   ``hostname.bind`` with type ``TXT`` and class ``CHAOS``. This defaults
   to the hostname of the machine hosting the name server, as found by
   the ``gethostname()`` function. The primary purpose of such queries is to
   identify which of a group of anycast servers is actually answering
   the queries. Specifying ``hostname none;`` disables processing of
   the queries.

.. namedconf:statement:: server-id
   :tags: server
   :short: Specifies the ID of the server to return in response to a ``ID.SERVER`` query.

   This is the ID the server should report when receiving a Name Server
   Identifier (NSID) query, or a query of the name ``ID.SERVER`` with
   type ``TXT`` and class ``CHAOS``. The primary purpose of such queries is
   to identify which of a group of anycast servers is actually answering
   the queries. Specifying ``server-id none;`` disables processing of
   the queries. Specifying ``server-id hostname;`` causes :iscman:`named`
   to use the hostname as found by the ``gethostname()`` function. The
   default :any:`server-id` is ``none``.

.. _empty:

Built-in Empty Zones
^^^^^^^^^^^^^^^^^^^^

The :iscman:`named` server has some built-in empty zones, for SOA and NS records
only. These are for zones that should normally be answered locally and for
which queries should not be sent to the Internet's root servers. The
official servers that cover these namespaces return NXDOMAIN responses
to these queries. In particular, these cover the reverse namespaces for
addresses from :rfc:`1918`, :rfc:`4193`, :rfc:`5737`, and :rfc:`6598`. They also
include the reverse namespace for the IPv6 local address (locally assigned),
IPv6 link local addresses, the IPv6 loopback address, and the IPv6
unknown address.

The server attempts to determine whether a built-in zone already exists
or is active (covered by a forward-only forwarding declaration), and does
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
-  RESOLVER.ARPA

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

.. namedconf:statement:: empty-server
   :tags: server, zone
   :short: Specifies the server name in the returned SOA record for empty zones.

   This specifies the server name that appears in the returned SOA record for
   empty zones. If none is specified, the zone's name is used.

.. namedconf:statement:: empty-contact
   :tags: server, zone
   :short: Specifies the contact name in the returned SOA record for empty zones.

   This specifies the contact name that appears in the returned SOA record for
   empty zones. If none is specified, "." is used.

.. namedconf:statement:: empty-zones-enable
   :tags: server, zone
   :short: Enables or disables all empty zones.

   This enables or disables all empty zones. By default, they are enabled.

.. namedconf:statement:: disable-empty-zone
   :tags: server, zone
   :short: Disables individual empty zones.

   This disables individual empty zones. By default, none are disabled. This
   option can be specified multiple times.

.. _content_filtering:

Content Filtering
^^^^^^^^^^^^^^^^^

.. namedconf:statement:: deny-answer-addresses
   :tags: query
   :short: Rejects A or AAAA records if the corresponding IPv4 or IPv6 addresses match a given :any:`address_match_list`.

   BIND 9 provides the ability to filter out responses from external
   DNS servers containing certain types of data in the answer section.
   Specifically, it can reject address (A or AAAA) records if the
   corresponding IPv4 or IPv6 addresses match the given
   :term:`address_match_list` of the :any:`deny-answer-addresses` option.

   In the :term:`address_match_list` of the :any:`deny-answer-addresses` option,
   only :term:`ip_address` and :term:`netprefix` are meaningful; any :term:`server_key` is
   silently ignored.

.. namedconf:statement:: deny-answer-aliases
   :tags: query
   :short: Rejects CNAME or DNAME records if the "alias" name matches a given list of :any:`domain_name` elements.

   BIND can
   also reject CNAME or DNAME records if the "alias" name (i.e., the CNAME
   alias or the substituted query name due to DNAME) matches the given
   list of :term:`domain_name` elements of the :any:`deny-answer-aliases` option,
   where "match" means the alias name is a subdomain of one of the listed domain names. If
   the optional list is specified in the ``except-from`` argument, records
   whose query name matches the list are accepted regardless of the
   filter setting. Likewise, if the alias name is a subdomain of the
   corresponding zone, the :any:`deny-answer-aliases` filter does not apply;
   for example, even if "example.com" is specified for
   :any:`deny-answer-aliases`,

   ::

      www.example.com. CNAME xxx.example.com.

   returned by an "example.com" server is accepted.

If a response message is rejected due to filtering, the entire
message is discarded without being cached and a SERVFAIL error is
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

.. namedconf:statement:: response-policy
   :tags: server, query, zone, security
   :short: Specifies response policy zones for the view or among global options.

   Response policy zones are named in the :any:`response-policy` option for
   the view, or among the global options if there is no :any:`response-policy`
   option for the view. Response policy zones are ordinary DNS zones
   containing RRsets that can be queried normally if allowed. It is usually
   best to restrict those queries with something like
   ``allow-query { localhost; };``.

   A :any:`response-policy` option can support multiple policy zones. To
   maximize performance, a radix tree is used to quickly identify response
   policy zones containing triggers that match the current query. This
   imposes an upper limit of 64 on the number of policy zones in a single
   :any:`response-policy` option; more than that is a configuration error.

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

   If authoritative name servers for the query name are not yet known, :iscman:`named`
   recursively looks up the authoritative servers for the query name before
   applying an RPZ-NSDNAME rule, which can cause a processing delay. To speed up
   processing at the cost of precision, the ``nsdname-wait-recurse`` option can
   be used; when set to ``no``, RPZ-NSDNAME rules are only applied when
   authoritative servers for the query name have already been looked up and
   cached.  If authoritative servers for the query name are not in the cache,
   the RPZ-NSDNAME rule is ignored, but the authoritative servers for
   the query name are looked up in the background and the rule is
   applied to subsequent queries. The default is ``yes``,
   meaning RPZ-NSDNAME rules are always applied, even if authoritative
   servers for the query name need to be looked up first.

``RPZ-NSIP``
   NSIP triggers match the IP addresses of authoritative servers. They
   are encoded like IP triggers, except as subdomains of ``rpz-nsip``.
   NSDNAME and NSIP triggers are checked only for names with at least
   ``min-ns-dots`` dots. The default value of ``min-ns-dots`` is 1, to
   exclude top-level domains. The ``nsip-enable`` phrase turns NSIP
   triggers off or on for a single policy zone or for all zones.

   If a name server's IP address is not yet known, :iscman:`named`
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
:any:`response-policy` option. An organization using a policy zone provided
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

Also by default, when :iscman:`named` is started it may start answering to
queries before the response policy zones are completely loaded and processed.
This can be changed with the ``servfail-until-ready yes`` option, in which case
incoming requests will result in SERVFAIL answer, until all the response policy
zones are ready. Note that if one or more response policy zones fail to load,
:iscman:`named` starts responding to queries according to those zones that did
load. Note, that enabling this option has no effect when a DNS Response Policy
Service (DNSRPS) interface is used.

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

.. namedconf:statement:: dnsrps-enable
   :tags: server, security
   :short: Turns on the DNS Response Policy Service (DNSRPS) interface.

   The ``dnsrps-enable yes`` option turns on the DNS Response Policy Service
   (DNSRPS) interface, if it has been compiled in :iscman:`named` using
   ``configure --enable-dnsrps``.

.. namedconf:statement:: dnsrps-library
   :tags: server, security
   :short: Specifies the path to the DNS Response Policy Service (DNSRPS) provider library.

   This option specifies the path to the DNSRPS provider library. Typically
   this library is detected when building with ``configure --enable-dnsrps``
   and does not need to be specified in ``named.conf``; the option exists
   to override the default library for testing purposes.

.. namedconf:statement:: dnsrps-options
   :tags: server, security
   :short: Provides additional RPZ configuration settings, which are passed to the DNS Response Policy Service (DNSRPS) provider library.

   The block provides additional RPZ configuration
   settings, which are passed through to the DNSRPS provider library.
   Multiple DNSRPS settings in an :any:`dnsrps-options` string should be
   separated with semi-colons (;). The DNSRPS provider library is passed a
   configuration string consisting of the :any:`dnsrps-options` text,
   concatenated with settings derived from the :any:`response-policy`
   statement.

   Note: the :any:`dnsrps-options` text should only include configuration
   settings that are specific to the DNSRPS provider. For example, the
   DNSRPS provider from Farsight Security takes options such as
   ``dnsrpzd-conf``, ``dnsrpzd-sock``, and ``dnzrpzd-args`` (for details of
   these options, see the ``librpz`` documentation). Other RPZ
   configuration settings could be included in :any:`dnsrps-options` as well,
   but if :iscman:`named` were switched back to traditional RPZ by setting
   :any:`dnsrps-enable` to "no", those options would be ignored.

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

Response policy zones can be configured to set an Extended DNS Error (EDE) code
on the responses which have been modified by the response policy:

::

       response-policy { zone "badlist" ede filtered; };

The following settings are supported for the ``ede`` option:

``none``
   No Extended DNS Error code is set (default).

``forged``
   Extended DNS Error code 4 - Forged Answer.

``blocked``
   Extended DNS Error code 15 - Blocked.

``censored``
   Extended DNS Error code 16 - Censored.

``filtered``
   Extended DNS Error code 17 - Filtered.

``prohibited``
   Extended DNS Error code 18 - Prohibited.

See :rfc:`8914` for more information about the Extended DNS Error codes.

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

.. namedconf:statement:: rate-limit
   :tags: query
   :short: Controls excessive UDP responses, to prevent BIND 9 from being used to amplify reflection denial-of-service (DoS) attacks.

   Excessive, almost-identical UDP *responses* can be controlled by
   configuring a :any:`rate-limit` clause in an :namedconf:ref:`options` or :any:`view`
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

   Response-rate limiting uses a "credit" or "token bucket" scheme. Each
   combination of identical response and client has a conceptual "account"
   that earns a specified number of credits every second. A prospective
   response debits its account by one. Responses are dropped or truncated
   while the account is negative.

   .. namedconf:statement:: window
      :tags: query
      :short: Specifies the length of time during which responses are tracked.

      Responses are tracked within a rolling
      window of time which defaults to 15 seconds, but which can be configured with
      the :any:`window` option to any value from 1 to 3600 seconds (1 hour). The
      account cannot become more positive than the per-second limit or more
      negative than :any:`window` times the per-second limit. When the specified
      number of credits for a class of responses is set to 0, those responses
      are not rate-limited.

   .. namedconf:statement:: ipv4-prefix-length
      :tags: server
      :short: Specifies the prefix lengths of IPv4 address blocks.

   .. namedconf:statement:: ipv6-prefix-length
      :tags: server
      :short: Specifies the prefix lengths of IPv6 address blocks.

      The notions of "identical response" and "DNS client" for rate limiting
      are not simplistic. All responses to an address block are counted as if
      to a single client. The prefix lengths of address blocks are specified
      with :any:`ipv4-prefix-length` (default 24) and :any:`ipv6-prefix-length`
      (default 56).

   .. namedconf:statement:: responses-per-second
      :tags: query
      :short: Limits the number of non-empty responses for a valid domain name and record type.

      All non-empty responses for a valid domain name (qname) and record type
      (qtype) are identical and have a limit specified with
      :any:`responses-per-second` (default 0 or no limit). All valid wildcard
      domain names are interpreted as the zone's origin name concatenated to the
      "*" name.

   .. namedconf:statement:: nodata-per-second
      :tags: query
      :short: Limits the number of empty (NODATA) responses for a valid domain name.

      All empty (NODATA)
      responses for a valid domain, regardless of query type, are identical.
      Responses in the NODATA class are limited by :any:`nodata-per-second`
      (default :any:`responses-per-second`).

   .. namedconf:statement:: nxdomains-per-second
      :tags: query
      :short: Limits the number of undefined subdomains for a valid domain name.

      Requests for any and all undefined
      subdomains of a given valid domain result in NXDOMAIN errors, and are
      identical regardless of query type. They are limited by
      :any:`nxdomains-per-second` (default :any:`responses-per-second`). This
      controls some attacks using random names, but can be relaxed or turned
      off (set to 0) on servers that expect many legitimate NXDOMAIN
      responses, such as from anti-spam rejection lists.

   .. namedconf:statement:: referrals-per-second
      :tags: query
      :short: Limits the number of referrals or delegations to a server for a given domain.

      Referrals or delegations
      to the server of a given domain are identical and are limited by
      :any:`referrals-per-second` (default :any:`responses-per-second`).

   Responses generated from local wildcards are counted and limited as if
   they were for the parent domain name. This controls flooding using
   random.wild.example.com.

   All requests that result in DNS errors other than NXDOMAIN, such as
   SERVFAIL and FORMERR, are identical regardless of requested name (qname)
   or record type (qtype). This controls attacks using invalid requests or
   distant, broken authoritative servers.


   .. namedconf:statement:: errors-per-second
      :tags: server
      :short: Limits the number of errors for a valid domain name and record type.

      By default the limit on errors is
      the same as the :any:`responses-per-second` value, but it can be set
      separately with :any:`errors-per-second`.

   .. namedconf:statement:: slip
      :tags: query
      :short: Sets the number of "slipped" responses to minimize the use of forged source addresses for an attack.

      Many attacks using DNS involve UDP requests with forged source
      addresses. Rate limiting prevents the use of BIND 9 to flood a network
      with responses to requests with forged source addresses, but could let a
      third party block responses to legitimate requests. There is a mechanism
      that can answer some legitimate requests from a client whose address is
      being forged in a flood. Setting :any:`slip` to 2 (its default) causes
      every other UDP request without a valid server cookie to be answered with
      a small response. The small size and reduced frequency, and resulting lack of
      amplification, of "slipped" responses make them unattractive for
      reflection DoS attacks. :any:`slip` must be between 0 and 10. A value of 0
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
      truncated responses and are instead leaked at the :any:`slip` rate.

      (Note: dropped responses from an authoritative server may reduce the
      difficulty of a third party successfully forging a response to a
      recursive resolver. The best security against forged responses is for
      authoritative operators to sign their zones using DNSSEC and for
      resolver operators to validate the responses. When this is not an
      option, operators who are more concerned with response integrity than
      with flood mitigation may consider setting :any:`slip` to 1, causing all
      rate-limited responses to be truncated rather than dropped. This reduces
      the effectiveness of rate-limiting against reflection attacks.)

   .. namedconf:statement:: qps-scale
      :tags: query
      :short: Tightens defenses during DNS attacks by scaling back the ratio of the current query-per-second rate.

      When the approximate query-per-second rate exceeds the :any:`qps-scale`
      value, the :any:`responses-per-second`, :any:`errors-per-second`,
      :any:`nxdomains-per-second`, and :any:`all-per-second` values are reduced by
      the ratio of the current rate to the :any:`qps-scale` value. This feature
      can tighten defenses during attacks. For example, with
      ``qps-scale 250; responses-per-second 20;`` and a total query rate of
      1000 queries/second for all queries from all DNS clients including via
      TCP, then the effective responses/second limit changes to (250/1000)*20,
      or 5. Responses to requests that included a valid server cookie,
      and responses sent via TCP, are not limited but are counted to compute
      the query-per-second rate.

   .. namedconf:statement:: exempt-clients
      :tags: query
      :short: Exempts specific clients or client groups from rate limiting.

      Communities of DNS clients can be given their own parameters or no
      rate limiting by putting :any:`rate-limit` statements in :any:`view` statements
      instead of in the global ``option`` statement. A :any:`rate-limit` statement
      in a view replaces, rather than supplements, a :any:`rate-limit`
      statement among the main options.

      DNS clients within a view can be
      exempted from rate limits with the :any:`exempt-clients` clause.

   .. namedconf:statement:: all-per-second
      :tags: query
      :short: Limits UDP responses of all kinds.

      UDP responses of all kinds can be limited with the :any:`all-per-second`
      phrase. This rate limiting is unlike the rate limiting provided by
      :any:`responses-per-second`, :any:`errors-per-second`, and
      :any:`nxdomains-per-second` on a DNS server, which are often invisible to
      the victim of a DNS reflection attack. Unless the forged requests of the
      attack are the same as the legitimate requests of the victim, the
      victim's requests are not affected. Responses affected by an
      :any:`all-per-second` limit are always dropped; the :any:`slip` value has no
      effect. An :any:`all-per-second` limit should be at least 4 times as large
      as the other limits, because single DNS clients often send bursts of
      legitimate requests. For example, the receipt of a single mail message
      can prompt requests from an SMTP server for NS, PTR, A, and AAAA records
      as the incoming SMTP/TCP/IP connection is considered. The SMTP server
      can need additional NS, A, AAAA, MX, TXT, and SPF records as it
      considers the SMTP ``Mail From`` command. Web browsers often repeatedly
      resolve the same names that are duplicated in HTML <IMG> tags in a page.
      :any:`all-per-second` is similar to the rate limiting offered by firewalls
      but is often inferior. Attacks that justify ignoring the contents of DNS
      responses are likely to be attacks on the DNS server itself. They
      usually should be discarded before the DNS server spends resources making
      TCP connections or parsing DNS requests, but that rate limiting must be
      done before the DNS server sees the requests.


   .. namedconf:statement:: max-table-size
      :tags: server
      :short: Sets the maximum size of the table used to track requests and rate-limit responses.

   .. namedconf:statement:: min-table-size
      :tags: query
      :short: Sets the minimum size of the table used to track requests and rate-limit responses.

      The maximum size of the table used to track requests and rate-limit
      responses is set with :any:`max-table-size`. Each entry in the table is
      between 40 and 80 bytes. The table needs approximately as many entries
      as the number of requests received per second. The default is 20,000. To
      reduce the cold start of growing the table, :any:`min-table-size` (default 500)
      can set the minimum table size. Enable :any:`rate-limit` category
      logging to monitor expansions of the table and inform choices for the
      initial and maximum table size.

   .. namedconf:statement:: log-only
      :tags: logging, query
      :short: Tests rate-limiting parameters without actually dropping any requests.

      Use ``log-only yes`` to test rate-limiting parameters without actually
      dropping any requests.

   Responses dropped by rate limits are included in the ``RateDropped`` and
   ``QryDropped`` statistics. Responses that are truncated by rate limits are
   included in ``RateSlipped`` and ``RespTruncated``.

NXDOMAIN Redirection
^^^^^^^^^^^^^^^^^^^^

:iscman:`named` supports NXDOMAIN redirection via two methods:

-  :any:`Redirect zone <type redirect>`
-  Redirect namespace

With either method, when :iscman:`named` gets an NXDOMAIN response it examines a
separate namespace to see if the NXDOMAIN response should be replaced
with an alternative response.

With a redirect zone (``zone "." { type redirect; };``), the data used
to replace the NXDOMAIN is held in a single zone which is not part of
the normal namespace. All the redirect information is contained in the
zone; there are no delegations.

.. namedconf:statement:: nxdomain-redirect
   :tags: query
   :short: Appends the specified suffix to the original query name, when replacing an NXDOMAIN with a redirect namespace.

   With a redirect namespace (``option { nxdomain-redirect <suffix> };``),
   the data used to replace the NXDOMAIN is part of the normal namespace
   and is looked up by appending the specified suffix to the original
   query name. This roughly doubles the cache required to process
   NXDOMAIN responses, as both the original NXDOMAIN response and the
   replacement data (or an NXDOMAIN indicating that there is no
   replacement) must be stored.

If both a redirect zone and a redirect namespace are configured, the
redirect zone is tried first.

``server`` Block Grammar
~~~~~~~~~~~~~~~~~~~~~~~~~~~~
.. namedconf:statement:: server
   :tags: server
   :short: Defines characteristics to be associated with a remote name server.

``server`` Block Definition and Usage
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The :namedconf:ref:`server` statement defines characteristics to be associated with a
remote name server. If a prefix length is specified, then a range of
servers is covered. Only the most specific server clause applies,
regardless of the order in :iscman:`named.conf`.

The :rndcconf:ref:`server` statement can occur at the top level of the configuration
file or inside a :any:`view` statement. If a :any:`view` statement contains
one or more :namedconf:ref:`server` statements, only those apply to the view and any
top-level ones are ignored. If a view contains no :namedconf:ref:`server` statements,
any top-level :namedconf:ref:`server` statements are used as defaults.


.. namedconf:statement:: bogus
   :tags: server
   :short: Allows a remote server to be ignored.

   If a remote server is giving out bad data, marking it
   as bogus prevents further queries to it. The default value of
   :any:`bogus` is ``no``.

.. namedconf:statement:: edns
   :tags: server
   :short: Controls the use of the EDNS0 (:rfc:`2671`) feature.

   The :any:`edns` clause determines whether the local server attempts to
   use EDNS when communicating with the remote server. The default is
   ``yes``.

.. namedconf:statement:: edns-version
   :tags: server
   :short: Sets the maximum EDNS VERSION that is sent to the server(s) by the resolver.

   The :any:`edns-version` option sets the maximum EDNS VERSION that is
   sent to the server(s) by the resolver. The actual EDNS version sent is
   still subject to normal EDNS version-negotiation rules (see :rfc:`6891`),
   the maximum EDNS version supported by the server, and any other
   heuristics that indicate that a lower version should be sent. This
   option is intended to be used when a remote server reacts badly to a
   given EDNS version or higher; it should be set to the highest version
   the remote server is known to support. Valid values are 0 to 255; higher
   values are silently adjusted. This option is not needed until
   higher EDNS versions than 0 are in use.

.. namedconf:statement:: padding
   :tags: server
   :short: Adds EDNS Padding options to outgoing messages to increase the packet size.

   The option adds EDNS Padding options to outgoing messages,
   increasing the packet size to a multiple of the specified block size.
   Valid block sizes range from 0 (the default, which disables the use of
   EDNS Padding) to 512 bytes. Larger values are reduced to 512, with a
   logged warning. Note: this option is not currently compatible with
   TSIG or SIG(0), as the EDNS OPT record containing the padding would have
   to be added to the packet after it had already been signed.

.. namedconf:statement:: tcp-only
   :tags: server
   :short: Sets the transport protocol to TCP.

   The option sets the transport protocol to TCP. The default
   is to use the UDP transport and to fallback on TCP only when a truncated
   response is received.

.. namedconf:statement:: tcp-keepalive
   :tags: server
   :short: Adds EDNS TCP keepalive to messages sent over TCP.

   The option adds EDNS TCP keepalive to messages sent
   over TCP. Note that currently idle timeouts in responses are ignored.

.. namedconf:statement:: transfers
   :tags: server
   :short: Limits the number of concurrent inbound zone transfers from a server.

   :any:`transfers` is used to limit the number of concurrent inbound zone
   transfers from the specified server. If no :any:`transfers` clause is
   specified, the limit is set according to the :any:`transfers-per-ns`
   option.

.. namedconf:statement:: keys
   :tags: server, security
   :short: Specifies one or more :any:`server_key` s to be used with a remote server.
   :suppress_grammar:

   .. warning::
      This option is not to be confused with :any:`keys` in the :any:`dnssec-policy` specification.
      Although statements with the same name exist in both contexts, they refer
      to fundamentally incompatible concepts.

   In the context of a :namedconf:ref:`server` block, the option identifies a
   :term:`server_key` defined by the :namedconf:ref:`key` statement, to be used for
   transaction security (see :ref:`tsig`)
   when talking to the remote server. When a request is sent to the remote
   server, a request signature is generated using the key specified
   here and appended to the message. A request originating from the remote
   server is not required to be signed by this key.

   Only a single key per server is currently supported.

It is possible to override the following values defined in :namedconf:ref:`view`
and :namedconf:ref:`options` blocks:

   - :namedconf:ref:`edns-udp-size`
   - :namedconf:ref:`max-udp-size`
   - :namedconf:ref:`notify-source-v6`
   - :namedconf:ref:`notify-source`
   - :namedconf:ref:`provide-ixfr`
   - :namedconf:ref:`query-source-v6`
   - :namedconf:ref:`query-source`
   - :namedconf:ref:`request-expire`
   - :namedconf:ref:`request-ixfr`
   - :namedconf:ref:`request-nsid`
   - :namedconf:ref:`require-cookie`
   - :namedconf:ref:`send-cookie`
   - :namedconf:ref:`transfer-format`
   - :namedconf:ref:`transfer-source-v6`
   - :namedconf:ref:`transfer-source`


:any:`statistics-channels` Block Grammar
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
.. namedconf:statement:: statistics-channels
   :tags: logging
   :short: Specifies the communication channels to be used by system administrators to access statistics information on the name server.

:any:`statistics-channels` Block Definition and Usage
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The :any:`statistics-channels` statement declares communication channels to
be used by system administrators to get access to statistics information
on the name server.

This statement is intended to be flexible to support multiple communication
protocols in the future, but currently only HTTP access is supported. It
requires that BIND 9 be compiled with libxml2 and/or json-c (also known
as libjson0); the :any:`statistics-channels` statement is still accepted
even if it is built without the library, but any HTTP access fails
with an error.

An :any:`inet` control channel is a TCP socket listening at the specified
:term:`port` on the specified :term:`ip_address`, which can be an IPv4 or IPv6
address. An :term:`ip_address` of ``*`` (asterisk) is interpreted as the IPv4
wildcard address; connections are accepted on any of the system's
IPv4 addresses. To listen on the IPv6 wildcard address, use an
:term:`ip_address` of ``::``.

If no port is specified, port 80 is used for HTTP channels. The asterisk
(``*``) cannot be used for :term:`port`.

Attempts to open a statistics channel are restricted by the
optional ``allow`` clause. Connections to the statistics channel are
permitted based on the :term:`address_match_list`. If no ``allow`` clause is
present, :iscman:`named` accepts connection attempts from any address. Since
the statistics may contain sensitive internal information, the source of
connection requests must be restricted appropriately so that only
trusted parties can access the statistics channel.

Gathering data exposed by the statistics channel locks various subsystems in
:iscman:`named`, which could slow down query processing if statistics data is
requested too often.

An issue in the statistics channel would be considered a security issue
only if it could be exploited by unprivileged users circumventing the access
control list. In other words, any issue in the statistics channel that could be
used to access information unavailable otherwise, or to crash :iscman:`named`, is
not considered a security issue if it can be avoided through the
use of a secure configuration.

If no :any:`statistics-channels` statement is present, :iscman:`named` does not
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
statistics), http://127.0.0.1:8888/xml/v3/xfrins (incoming zone transfer
statistics), http://127.0.0.1:8888/xml/v3/net (network status and socket
statistics), http://127.0.0.1:8888/xml/v3/mem (memory manager
statistics), and http://127.0.0.1:8888/xml/v3/traffic (traffic sizes).

The full set of statistics can also be read in JSON format at
http://127.0.0.1:8888/json, with the broken-out subsets at
http://127.0.0.1:8888/json/v1/status (server uptime and last
reconfiguration time), http://127.0.0.1:8888/json/v1/server (server and
resolver statistics), http://127.0.0.1:8888/json/v1/zones (zone
statistics), http://127.0.0.1:8888/json/v1/xfrins (incoming zone transfer
statistics), http://127.0.0.1:8888/json/v1/net (network status and
socket statistics), http://127.0.0.1:8888/json/v1/mem (memory manager
statistics), and http://127.0.0.1:8888/json/v1/traffic (traffic sizes).

:any:`tls` Block Grammar
~~~~~~~~~~~~~~~~~~~~~~~~~
.. namedconf:statement:: tls
   :tags: security
   :short: Configures a TLS connection.

:any:`tls` Block Definition and Usage
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The :any:`tls` statement is used to configure a TLS connection; this
configuration can then be referenced by a :any:`listen-on` or :any:`listen-on-v6`
statement to cause :iscman:`named` to listen for incoming requests via TLS,
or in the :any:`primaries` statement for a zone of :any:`type secondary` to
cause zone transfer requests to be sent via TLS.

:any:`tls` can only be set at the top level of :iscman:`named.conf`.

The following options can be specified in a :any:`tls` statement:

.. namedconf:statement:: key-file
   :tags: server, security
   :short: Specifies the path to a file containing the private TLS key for a connection.

    This indicates the path to a file containing the private TLS key to be used for
    the connection.

.. namedconf:statement:: cert-file
   :tags: server, security
   :short: Specifies the path to a file containing the TLS certificate for a connection.

    This indicates the path to a file containing the TLS certificate to be used for
    the connection.

.. namedconf:statement:: ca-file
   :tags: server, security
   :short: Specifies the path to a file containing TLS certificates for trusted CA authorities, used to verify remote peer certificates.

    This indicates the path to a file containing trusted CA authorities' TLS
    certificates, used to verify remote peer certificates. Specifying
    this option enables verification of remote peer certificates. For
    incoming connections, specifying this option makes BIND require
    a valid TLS certificate from a client. In the case of outgoing
    connections, if :any:`remote-hostname` is not specified, the remote
    server IP address is used instead.

.. namedconf:statement:: dhparam-file
   :tags: server, security
   :short: Specifies the path to a file containing Diffie-Hellman parameters, for enabling cipher suites.

    This indicates the path to a file containing Diffie-Hellman parameters,
    which is needed to enable the cipher suites depending on the
    Diffie-Hellman ephemeral key exchange (DHE). Having these parameters
    specified is essential for enabling perfect forward secrecy capable
    ciphers in TLSv1.2.

.. namedconf:statement:: remote-hostname
   :tags: security
   :short: Specifies the expected hostname in the TLS certificate of the remote server.

    This specifies the expected hostname in the TLS certificate of the
    remote server. This option enables a remote server certificate
    verification. If :any:`ca-file` is not specified, then the
    platform-specific certificates store is used for
    verification. This option is used when connecting to a remote peer
    only and, thus, is ignored when :any:`tls` statements are referenced
    by :any:`listen-on` or :any:`listen-on-v6` statements.

.. namedconf:statement:: protocols
   :tags: security
   :short: Specifies the allowed versions of the TLS protocol.

    This specifies the allowed versions of the TLS protocol. TLS version 1.2 and higher are
    supported, depending on the cryptographic library in use. Multiple
    versions may be specified (e.g.
    ``protocols { TLSv1.2; TLSv1.3; };``).

.. namedconf:statement:: cipher-suites
   :tags: security
   :short: Specifies a list of allowed cipher suites in the order of preference for TLSv1.3 only.

    This option defines allowed cipher suites, such as
    ``TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:TLS_AES_128_GCM_SHA256``.
    The string must be formed according to the rules specified in the
    OpenSSL documentation (see
    https://docs.openssl.org/1.1.1/man1/ciphers/, section
    "TLS v1.3 cipher suites" for details).

.. namedconf:statement:: ciphers
   :tags: security
   :short: Specifies a list of allowed ciphers in the order of preference for TLSv1.2 only.

    This option defines allowed ciphers, such as
    ``HIGH:!aNULL:!MD5:!SHA1:!SHA256:!SHA384``. The string must be
    formed according to the rules specified in the OpenSSL documentation
    (see https://docs.openssl.org/1.1.1/man1/ciphers/
    for details).

.. namedconf:statement:: prefer-server-ciphers
   :tags: server, security
   :short: Specifies that server ciphers should be preferred over client ones.

    This option specifies that server ciphers should be preferred over client ones.

.. namedconf:statement:: session-tickets
   :tags: security
   :short: Enables or disables session resumption through TLS session tickets.

    This option enables or disables session resumption through TLS session tickets,
    as defined in :rfc:`5077`. Disabling the stateless session tickets
    might be required in the cases when forward secrecy is needed,
    or the TLS certificate and key pair is planned to be used across
    multiple BIND instances.

.. warning::

   TLS configuration is subject to change and incompatible changes might
   be introduced in the future. Users of TLS are encouraged to carefully
   read release notes when upgrading.

The options described above are used to control different aspects of
TLS functioning. Thus, most of them have no well-defined default
values, as these depend on the cryptographic library version in use
and system-wide cryptographic policy. On the other hand, by specifying
the needed options one could have a uniform configuration deployable
across a range of platforms.

An example of privacy-oriented, perfect forward secrecy enabled
configuration can be found below. It can be used as a
starting point.

::

   tls local-tls {
       key-file "/path/to/key.pem";
       cert-file "/path/to/fullchain_cert.pem";
       dhparam-file "/path/to/dhparam.pem";
       ciphers "HIGH:!kRSA:!aNULL:!eNULL:!RC4:!3DES:!MD5:!EXP:!PSK:!SRP:!DSS:!SHA1:!SHA256:!SHA384";
       prefer-server-ciphers yes;
       session-tickets no;
   };

A Diffie-Hellman parameters file can be generated using e.g. OpenSSL,
like follows:

::

   openssl dhparam -out /path/to/dhparam.pem <3072_or_4096>

It is important to ensure that the file is generated on a machine with enough entropy from
external sources (e.g. the local computer should be fine,
the remote virtual machine or server might not be). These files do
not contain any sensitive data and can be shared if required.

There are two built-in TLS connection configurations: ``ephemeral``, which
uses a temporary key and certificate created for the current :iscman:`named`
session only, and ``none``, which can be used when setting up an HTTP
listener with no encryption.

The main motivation behind the existence of the ``ephemeral`` configuration is
to aid in testing. Since trusted certificate authorities do not issue the
certificates associated with this configuration, these
certificates will never be trusted by any clients that verify TLS
certificates; they provide encryption of the traffic but no
authentication of the transmission channel. That might be enough in
the case of deployment in a controlled environment.

It should be noted that on reconfiguration, the ``ephemeral`` TLS key
and the certificate are recreated, and all TLS certificates and keys,
as well as associated data, are reloaded from the disk. In that case,
listening sockets associated with TLS remain intact.

Note that performing a reconfiguration can cause a short
interruption in BIND's ability to process inbound client packets. The
length of interruption is environment- and configuration-specific. A
good example of when reconfiguration is necessary is when TLS keys and
certificates are updated on the disk.

BIND supports the following TLS authentication mechanisms described in
:rfc:`9103`, Section 9.3: Opportunistic TLS, Strict TLS, and Mutual
TLS.

.. _opportunistic-tls:

Opportunistic TLS provides encryption for data but does not provide
any authentication for the channel. This mode is the default and
is used whenever the :any:`remote-hostname` and :any:`ca-file` options are not set
in :any:`tls` statements in use. :rfc:`9103` allows optional fallback to
clear-text DNS in the cases when TLS is not available; however, BIND
intentionally does not support that fallback, to protect from
unexpected data leaks due to misconfiguration. Both BIND and its
complementary tools either successfully establish a secure channel via
TLS when instructed to do so, or fail to establish a connection
otherwise.

.. _strict-tls:

Strict TLS provides server authentication via a pre-configured
hostname for outgoing connections. This mechanism offers both channel
confidentiality and channel authentication (of the server). In order
to achieve Strict TLS, one needs to use :any:`remote-hostname` and, optionally,
:any:`ca-file` options in the :any:`tls` statements used for establishing
outgoing connections (e.g. the ones used to download zone from
primaries via TLS). Providing any of the mentioned options will enable
server authentication. If :any:`remote-hostname` is provided but :any:`ca-file` is
missing, then the platform-specific certificate authority certificates
are used for authentication. The set roughly corresponds to the one
used by WEB-browsers to authenticate HTTPS hosts. On the other hand,
if :any:`ca-file` is provided but :any:`remote-hostname` is missing, then the
remote side's IP address is used instead.

.. _mutual-tls:

Mutual TLS is an extension to Strict TLS that provides channel
confidentiality and mutual channel authentication. It builds up upon
the clients offering client certificates when establishing connections
and them doing the server authentication as in the case of Strict
TLS. The server verifies the provided client certificates and accepts
the TLS connection in case of successful verification or rejects it
otherwise. In order to instruct the server to require and verify
client TLS certificates, one needs to specify the :any:`ca-file` option
in :any:`tls` configurations used to configure server listeners. The
provided file must contain certificate authority certificates used to
issue client certificates. In most cases, one should build one's own
TLS certificate authority specifically to issue client certificates
and include the certificate authority certificate into the file.

For authenticating zone transfers over TLS, Mutual TLS might be
considered a standalone solution, while Strict TLS paired with
TSIG-based authentication and, optionally, IP-based access lists,
might be considered acceptable for most practical purposes. Mutual TLS
has the advantage of not requiring TSIG and thus, not having security
issues related to shared cryptographic secrets.

:any:`http` Block Grammar
~~~~~~~~~~~~~~~~~~~~~~~~~~
.. namedconf:statement:: http
   :tags: server, query
   :short: Configures HTTP endpoints on which to listen for DNS-over-HTTPS (DoH) queries.

:any:`http` Block Definition and Usage
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The :any:`http` statement is used to configure HTTP endpoints on which
to listen for DNS-over-HTTPS (DoH) queries. This configuration can
then be referenced by a :any:`listen-on` or :any:`listen-on-v6` statement to
cause :iscman:`named` to listen for incoming requests over HTTPS.

:any:`http` can only be set at the top level of :iscman:`named.conf`.

The following options can be specified in an :any:`http` statement:

.. namedconf:statement:: endpoints
   :tags: server, query
   :short: Specifies a list of HTTP query paths on which to listen.

    This specifies a list of HTTP query paths on which to listen. This is the portion
    of an :rfc:`3986`-compliant URI following the hostname; it must be
    an absolute path, beginning with "/". The default value
    is ``"/dns-query"``, if omitted.

.. namedconf:statement:: listener-clients
   :tags: server, query
   :short: Specifies a per-listener quota for active connections.

    This option specifies a per-listener quota for active connections.

.. namedconf:statement:: streams-per-connection
   :tags: server, query
   :short: Specifies the maximum number of concurrent HTTP/2 streams over an HTTP/2 connection.

    This option specifies the hard limit on the number of concurrent
    HTTP/2 streams over an HTTP/2 connection.

Any of the options above could be omitted. In such a case, a global value
specified in the :namedconf:ref:`options` statement is used
(see :any:`http-listener-clients`, :any:`http-streams-per-connection`.

For example, the following configuration enables DNS-over-HTTPS queries on
all local addresses:

::

   http local {
       endpoints { "/dns-query"; };
   };

   options {
       ....
       listen-on tls ephemeral http local { any; };
       listen-on-v6 tls ephemeral http local { any; };
   };


:any:`trust-anchors` Block Grammar
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
.. namedconf:statement:: trust-anchors
   :tags: dnssec
   :short: Defines :ref:`DNSSEC` trust anchors.

:any:`trust-anchors` Block Definition and Usage
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The :any:`trust-anchors` statement defines DNSSEC trust anchors. DNSSEC is
described in :ref:`DNSSEC`.

A trust anchor is defined when the public key or public key digest for a non-authoritative
zone is known but cannot be securely obtained through DNS, either
because it is the DNS root zone or because its parent zone is unsigned.
Once a key or digest has been configured as a trust anchor, it is treated as if it
has been validated and proven secure.

The resolver attempts DNSSEC validation on all DNS data in subdomains of
configured trust anchors. Validation below specified names can be
temporarily disabled by using :option:`rndc nta`, or permanently disabled with
the :any:`validate-except` option.

All keys listed in :any:`trust-anchors`, and their corresponding zones, are
deemed to exist regardless of what parent zones say. Only keys
configured as trust anchors are used to validate the DNSKEY RRset for
the corresponding name. The parent's DS RRset is not used.

:any:`trust-anchors` may be set at the top level of :iscman:`named.conf` or within
a view. If it is set in both places, the configurations are additive;
keys defined at the top level are inherited by all views, but keys
defined in a view are only used within that view.

The :any:`trust-anchors` statement can contain
multiple trust-anchor entries, each consisting of a
domain name, followed by an "anchor type" keyword indicating
the trust anchor's format, followed by the key or digest data.

If the anchor type is ``static-key`` or
``initial-key``, it is followed with the
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
deprecated :any:`trusted-keys` statement.)

Suppose, for example, that a zone's key-signing key was compromised, and
the zone owner had to revoke and replace the key. A resolver which had
the original key
configured using ``static-key`` or
``static-ds`` would be unable to validate
this zone any longer; it would reply with a SERVFAIL response
code. This would continue until the resolver operator had
updated the :any:`trust-anchors` statement with
the new key.

If, however, the trust anchor had been configured using
``initial-key`` or ``initial-ds``
instead, the zone owner could add a "stand-by" key to
the zone in advance. :iscman:`named` would store
the stand-by key, and when the original key was revoked,
:iscman:`named` would be able to transition smoothly
to the new key. It would also recognize that the old key had
been revoked and cease using that key to validate answers,
minimizing the damage that the compromised key could do.
This is the process used to keep the ICANN root DNSSEC key
up-to-date.

Whereas ``static-key`` and
``static-ds`` trust anchors continue
to be trusted until they are removed from
:iscman:`named.conf`, an
``initial-key`` or ``initial-ds``
is only trusted *once*: for as long as it
takes to load the managed key database and start the
:rfc:`5011` key maintenance process.

It is not possible to mix static with initial trust anchors
for the same domain name.

The first time :iscman:`named` runs with an
``initial-key`` or ``initial-ds``
configured in :iscman:`named.conf`, it fetches the
DNSKEY RRset directly from the zone apex,
and validates it
using the trust anchor specified in :any:`trust-anchors`.
If the DNSKEY RRset is validly signed by a key matching
the trust anchor, then it is used as the basis for a new
managed-keys database.

From that point on, whenever :iscman:`named` runs, it sees the ``initial-key`` or ``initial-ds``
listed in :any:`trust-anchors`, checks to make sure :rfc:`5011` key maintenance
has already been initialized for the specified domain, and if so,
simply moves on. The key specified in the :any:`trust-anchors` statement is
not used to validate answers; it is superseded by the key or keys stored
in the managed-keys database.

The next time :iscman:`named` runs after an ``initial-key`` or
``initial-ds`` has been *removed* from the :any:`trust-anchors` statement
(or changed to a ``static-key`` or ``static-ds``), the corresponding zone
is removed from the managed-keys database, and :rfc:`5011` key maintenance
is no longer used for that domain.

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
usually within 30 seconds. Whenever :iscman:`named` is using
automatic key maintenance, the zone file and journal file can be
expected to exist in the working directory. (For this reason, among
others, the working directory should be always be writable by
:iscman:`named`.)

If the :any:`dnssec-validation` option is set to ``auto``, :iscman:`named`
automatically sets up an ``initial-key`` for the root zone. This
initializing key is built into :iscman:`named` and is current as of the
release date.  When the root zone key changes, a running server detects
the change and rolls to the new key; however, newly installed servers being run
for the first time will need to be on a recent-enough version of BIND to
have been built with the current key.

:any:`dnssec-policy` Block Grammar
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
.. namedconf:statement:: dnssec-policy
   :tags: dnssec
   :short: Defines a key and signing policy (KASP) for zones.

:any:`dnssec-policy` Block Definition and Usage
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The :any:`dnssec-policy` statement defines a key and signing policy (KASP)
for zones.

A KASP determines how one or more zones are signed with DNSSEC.  For
example, it specifies how often keys should roll, which cryptographic
algorithms to use, and how often RRSIG records need to be refreshed.
Multiple key and signing policies can be configured with unique policy names.

A policy for a zone is selected using a :any:`dnssec-policy` statement in the
:namedconf:ref:`zone` block, specifying the name of the policy that should be
used.

There are three built-in policies:
  - ``default``, which uses the :ref:`default policy <dnssec_policy_default>`;
  - ``insecure``, to be used when the zone should be unsigned gracefully; and
  - ``none``, which means no DNSSEC policy (the same as not selecting
    :any:`dnssec-policy` at all; the zone is not signed).

Keys are not shared among zones, which means that one set of keys per
zone is generated even if they have the same policy.  If multiple views
are configured with different versions of the same zone, each separate
version uses the same set of signing keys.

If the expected key files that were previously observed have gone missing or
are inaccessible, key management is halted. This will prevent rollovers
from being started if there is a temporary file access issue. If his problem
is permanent it will eventually lead to expired signatures in your zone.
Note that if the key files are missing or inaccessible during :iscman:`named`
startup, BIND 9 will try to generate new keys according to the DNSSEC policy,
because it has no cached information about existing keys yet.

The :any:`dnssec-policy` statement requires dynamic DNS to be set up, or
:any:`inline-signing` to be enabled (which is the default for DNSSEC zones).

If :any:`inline-signing` is enabled, this means that a signed version of the
zone is maintained separately and is written out to a different file on disk
(the zone's filename plus a ``.signed`` extension).

If :any:`inline-signing` is disabled, the zone needs to be configured with
an :any:`update-policy` or :any:`allow-update`. In such a case, the DNSSEC
records are written to the filename set in the original zone's :any:`file`.

Key rollover timing is computed for each key according to the key
lifetime defined in the KASP. The lifetime may be modified by zone TTLs
and propagation delays, to prevent validation failures. When a key
reaches the end of its lifetime, :iscman:`named` generates and publishes a new
key automatically, then deactivates the old key and activates the new
one; finally, the old key is retired according to a computed schedule.

Zone-signing key (ZSK) rollovers require no operator input.  Key-signing
key (KSK) and combined-signing key (CSK) rollovers require action to be
taken to submit a DS record to the parent.  Rollover timing for KSKs and
CSKs is adjusted to take into account delays in processing and
propagating DS updates.

.. _dnssec_policy_default:

The policy ``default`` causes the zone to be signed with a single combined-signing
key (CSK) using the algorithm ECDSAP256SHA256; this key has an unlimited
lifetime. This policy can be displayed using the command :option:`named -C`.

.. note:: The default signing policy may change in future releases.
   This could require changes to a signing policy when upgrading to a
   new version of BIND. Check the release notes carefully when
   upgrading to be informed of such changes. To prevent policy changes
   on upgrade, use an explicitly defined :any:`dnssec-policy`, rather than
   ``default``.

If a :any:`dnssec-policy` statement is modified and the server restarted or
reconfigured, :iscman:`named` attempts to change the policy smoothly from the
old one to the new. For example, if the key algorithm is changed, then
a new key is generated with the new algorithm, and the old algorithm is
retired when the existing key's lifetime ends.

.. note:: Rolling to a new policy while another key rollover is already
   in progress is not yet supported, and may result in unexpected
   behavior.

The following options can be specified in a :any:`dnssec-policy` statement:

.. namedconf:statement:: cdnskey
   :tags: dnssec
   :short: Specifies whether a CDNSKEY record should be published during KSK rollover.

    When set to the default value of ``yes``, a CDNSKEY record is published
    during KSK rollovers when the DS of the successor key may be submitted to
    the parent.

.. namedconf:statement:: cds-digest-types
   :tags: dnssec
   :short: Specifies the digest types to use for CDS resource records.

    This indicates the digest types to use when generating CDS resource
    records. The default is SHA-256 only.

.. namedconf:statement:: dnskey-ttl
   :tags: dnssec
   :short: Specifies the time-to-live (TTL) for DNSKEY resource records.

    This indicates the TTL to use when generating DNSKEY resource
    records. The default is 1 hour (3600 seconds).

.. _dnssec-policy-inline-signing:

inline-signing
   :tags: dnssec
   :short: Specifies whether BIND 9 maintains a separate signed version of a zone.

   If ``yes``, BIND 9 maintains a separate signed version of the zone.
   An unsigned zone is transferred in or loaded from disk and the signed
   version of the zone is served with, possibly, a different serial
   number. The signed version of the zone is stored in a file that is
   the zone's filename (set in :any:`file`) with a ``.signed`` extension.

   This behavior is enabled by default.

.. _dnssec-policy-keys:

keys
   :tags: dnssec
   :short: Specifies the type of keys to be used for DNSSEC signing.

    This is a list specifying the algorithms and roles to use when
    generating keys and signing the zone. Entries in this list do not
    represent specific DNSSEC keys, which may be changed on a regular
    basis, but the roles that keys play in the signing policy. For
    example, configuring a KSK of algorithm RSASHA256 ensures that the
    DNSKEY RRset always includes a key-signing key for that algorithm.

    Here is an example (for illustration purposes only) of some possible
    entries in a ``keys`` list:

    ::

        keys {
            ksk key-directory lifetime unlimited algorithm rsasha256 2048;
            zsk lifetime 30d algorithm 8 tag-range 0 32767;
            csk key-store "hsm" lifetime P6MT12H3M15S algorithm ecdsa256;
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
    The two available options are ``key-store <string>`` and
    ``key-directory``.

    When using ``key-store``, the referenced :any:`key-store` describes
    how the key should be be stored. This can be as a file, or it can be
    inside a PKCS#11 token.

    When using ``key-directory``, the key is stored in the zone's
    configured :any:`key-directory`. This is also the default.

    When using ``tag-range``, valid key tags for managed keys are
    restricted to this range [``tag-min`` ``tag-max``].  The optional
    ``tag-range`` is intended to be used in multi-signer scenarios.
    The default is unlimited ([0..65535]).

    The ``lifetime`` parameter specifies how long a key may be used
    before rolling over. For convenience, TTL-style time-unit suffixes
    can be used to specify the key lifetime. It also accepts ISO 8601
    duration formats.

    In the example above, the first key has an
    unlimited lifetime, the second key may be used for 30 days, and the
    third key has a rather peculiar lifetime of 6 months, 12 hours, 3
    minutes, and 15 seconds.  A lifetime of 0 seconds is the same as
    ``unlimited``.

    Note that the lifetime of a key may be extended if retiring it too
    soon would cause validation failures. The key lifetime must be
    longer than the time it takes to do a rollover; that is, the lifetime
    must be more than the publication interval (which is the sum of
    :any:`dnskey-ttl`, :any:`publish-safety`, and :any:`zone-propagation-delay`).
    It must also be more than the retire interval (which is the sum of
    :any:`max-zone-ttl`, :any:`retire-safety`, :any:`zone-propagation-delay`,
    and signing delay (:any:`signatures-validity` minus
    :any:`signatures-refresh`) for ZSKs, and the sum of :any:`parent-ds-ttl`,
    :any:`retire-safety`, and :any:`parent-propagation-delay` for KSKs and
    CSKs). BIND 9 treats a key lifetime that is too short as an error.

    The ``algorithm`` parameter specifies the key's algorithm, expressed
    either as a string ("rsasha256", "ecdsa384", etc.) or as a decimal
    number.  An optional second parameter specifies the key's size in
    bits.  If it is omitted, as shown in the example for the second and
    third keys, an appropriate default size for the algorithm is used.
    Each KSK/ZSK pair must have the same algorithm. A CSK combines the
    functionality of a ZSK and a KSK.

.. note:: When changing the ``key-directory`` or the ``key-store``, BIND will
   be unable to find existing key files. Be sure to copy key files to the
   new directory before changing the path used in the configuration file.
   This is also true when changing to a built-in policy, e.g. to
   ``insecure``. In this specific case, the existing key files should be moved
   to the zone's ``key-directory`` from the new configuration.

.. namedconf:statement:: manual-mode
   :tags: dnssec
   :short: Run key management in a manual mode.

    If enabled, BIND 9 does not automatically start and progress key rollovers,
    instead the change is logged. Only after manual confirmation with
    :option:`rndc dnssec -step <rndc dnssec>` the change is made.

    This feature is off by default.

.. namedconf:statement:: offline-ksk
   :tags: dnssec
   :short: Specifies whether the DNSKEY, CDS, and CDNSKEY RRsets are being signed offline.

    If enabled, BIND 9 does not generate signatures for the DNSKEY, CDS, and
    CDNSKEY RRsets. Instead, the signed DNSKEY, CDS and CDNSKEY RRsets are
    looked up from Signed Key Response (SKR) files.

    Any existing DNSKEY, CDS, and CDNSKEY RRsets in the unsigned version of the
    zone are filtered and replaced with RRsets from the SKR file.

    This feature is off by default. Configuring ``offline-ksk`` in conjunction
    with a CSK is a configuration error.

.. namedconf:statement:: purge-keys
   :tags: dnssec
   :short: Specifies the amount of time after which DNSSEC keys that have been deleted from the zone can be removed from disk.

    This is the amount of time after which DNSSEC keys that have been deleted from
    the zone can be removed from disk. If a key still determined to have
    presence (for example in some resolver cache), :iscman:`named` will not
    remove the key files.

    The default is ``P90D`` (90 days). Set this option to ``0`` to never
    purge deleted keys.

.. namedconf:statement:: publish-safety
   :tags: dnssec
   :short: Increases the amount of time between when keys are published and when they become active, to allow for unforeseen events.

    This is a margin that is added to the pre-publication interval in
    rollover timing calculations, to give some extra time to cover
    unforeseen events.  This increases the time between when keys are
    published and when they become active.  The default is ``PT1H`` (1
    hour).

.. namedconf:statement:: retire-safety
   :tags: dnssec
   :short: Increases the amount of time a key remains published after it is no longer active, to allow for unforeseen events.

    This is a margin that is added to the post-publication interval in
    rollover timing calculations, to give some extra time to cover
    unforeseen events.  This increases the time a key remains published
    after it is no longer active.  The default is ``PT1H`` (1 hour).

.. namedconf:statement:: signatures-jitter
   :tags: dnssec
   :short: Specifies a range for signature expirations.

    To prevent all signatures from expiring at the same moment, BIND 9 may
    vary the validity interval of individual signatures. The validity of a
    newly generated signature is in the range between :any:`signatures-validity`
    (maximum) and :any:`signatures-validity`, minus :any:`signatures-jitter`
    (minimum). The default jitter is 12 hours, and the configured value must
    be lower than both :any:`signatures-validity` and
    :any:`signatures-validity-dnskey`.

.. namedconf:statement:: signatures-refresh
   :tags: dnssec
   :short: Specifies how frequently an RRSIG record is refreshed.

    This determines how frequently an RRSIG record needs to be
    refreshed. The signature is renewed when the time until the
    expiration time is less than the specified interval.  The default is
    ``P5D`` (5 days), meaning signatures that expire in 5 days or sooner
    are refreshed. The :any:`signatures-refresh` value must be less than
    90% of the minimum value of :any:`signatures-validity` and
    :any:`signatures-validity-dnskey`.

.. namedconf:statement:: signatures-validity
   :tags: dnssec
   :short: Indicates the validity period of an RRSIG record.

    This indicates the validity period of an RRSIG record (subject to
    inception offset and jitter). The default is ``P2W`` (2 weeks).

    The :any:`signatures-validity` should be at least several multiples
    of the SOA expire interval, to allow for reasonable interaction between
    the various timer and expiry dates.

.. namedconf:statement:: signatures-validity-dnskey
   :tags: dnssec
   :short: Indicates the validity period of DNSKEY records.

    This is similar to :any:`signatures-validity`, but for DNSKEY records.
    The default is ``P2W`` (2 weeks).

.. _dnssec-policy-max-zone-ttl:

max-zone-ttl
   :tags: zone, query
   :short: Specifies a maximum permissible time-to-live (TTL) value, in seconds.

   This specifies the maximum permissible TTL value for the zone.  When
   a zone file is loaded, any record encountered with a TTL higher than
   :any:`max-zone-ttl` causes the zone to be rejected.

   This ensures that when rolling to a new DNSKEY, the old key will remain
   available until RRSIG records have expired from caches. The
   :any:`max-zone-ttl` option guarantees that the largest TTL in the
   zone is no higher than a known and predictable value.

   The default value ``PT24H`` (24 hours). A value of zero is treated
   as if the default value were in use.

.. namedconf:statement:: nsec3param
   :tags: dnssec
   :short: Specifies the use of NSEC3 instead of NSEC, and sets NSEC3 parameters.

    Use NSEC3 instead of NSEC, and optionally set the NSEC3 parameters.

    Here is an example of an ``nsec3`` configuration:

    ::

        nsec3param iterations 0 optout no salt-length 0;

    The default is to use :ref:`NSEC`. The ``iterations``, ``optout``, and
    ``salt-length`` parts are optional, but if not set, the values in
    the example above are the default :ref:`NSEC3` parameters. Note that the
    specific salt string is not specified by the user; :iscman:`named` creates a salt
    of the indicated length.

    .. warning::
       Do not use extra :term:`iterations <Iterations>`, :term:`salt <Salt>`, and
       :term:`opt-out <Opt-out>` unless their implications are fully understood.
       A higher number of iterations causes interoperability problems and opens
       servers to CPU-exhausting DoS attacks. See :rfc:`9276`.

.. namedconf:statement:: zone-propagation-delay
   :tags: dnssec, zone
   :short: Sets the propagation delay from the time a zone is first updated to when the new version of the zone is served by all secondary servers.

    This is the expected propagation delay from the time when a zone is
    first updated to the time when the new version of the zone is served
    by all secondary servers. The default is ``PT5M`` (5 minutes).

.. namedconf:statement:: parent-ds-ttl
   :tags: dnssec
   :short: Sets the time to live (TTL) of the DS RRset used by the parent zone.

    This is the TTL of the DS RRset that the parent zone uses.  The
    default is ``P1D`` (1 day).

.. namedconf:statement:: parent-propagation-delay
   :tags: dnssec, zone
   :short: Sets the propagation delay from the time the parent zone is updated to when the new version is served by all of the parent zone's name servers.

    This is the expected propagation delay from the time when the parent
    zone is updated to the time when the new version is served by all of
    the parent zone's name servers. The default is ``PT1H`` (1 hour).

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

.. namedconf:statement:: parental-agents
   :tags: dnssec

   This specifies a list of one or more IP addresses of parental agents that
   are used to query the zone's DS records during a KSK rollover. The list of
   parental agents can also contain the names of :any:`remote-servers` blocks.

   By default, DS queries are sent from port 53 on the servers; this can be
   changed for all servers by specifying a port number before the list of IP
   addresses, or on a per-server basis after the IP address. Authentication to
   the primary can also be done with per-server TSIG keys.

The following options apply to DS queries sent to :any:`parental-agents`:

.. namedconf:statement:: checkds
   :tags: dnssec
   :short: Controls whether ``DS`` queries are sent to parental agents.

   If set to ``yes``, DS queries are sent when a KSK rollover is in progress.
   The queries are sent to the servers listed in the parent zone's NS records.
   This is the default if there are no :any:`parental-agents` configured for
   the zone.

   If set to ``explicit``, DS queries are sent only to servers explicitly listed
   using :any:`parental-agents`. This is the default if there are parental
   agents configured.

   If set to ``no``, no DS queries are sent. Users should manually run
   :option:`rndc dnssec -checkds <rndc dnssec>` with the appropriate parameters,
   to signal that specific DS records are published and/or withdrawn.

.. namedconf:statement:: parental-source
   :tags: dnssec
   :short: Specifies which local IPv4 source address is used to send parental DS queries.

   :any:`parental-source` determines which local source address, and optionally
   UDP port, is used to send parental DS queries. This statement sets the
   :any:`parental-source` for all zones, but can be overridden on a per-zone or
   per-view basis by including a :any:`parental-source` statement within the
   :any:`zone` or :any:`view` block in the configuration file.

   .. note:: ``port`` configuration is deprecated. A warning will be logged
      when this parameter is used.

   .. warning:: Specifying a single port is discouraged, as it removes a layer of
      protection against spoofing errors.

   .. warning:: The configured :term:`port` must not be the same as the listening port.

.. namedconf:statement:: parental-source-v6
   :tags: dnssec
   :short: Specifies which local IPv6 source address is used to send parental DS queries.

   This option acts like :any:`parental-source`, but applies to parental DS
   queries sent to IPv6 addresses.

:any:`managed-keys` Block Grammar
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
.. namedconf:statement:: managed-keys
   :tags: deprecated

:any:`managed-keys` Block Definition and Usage
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The :any:`managed-keys` statement has been
deprecated in favor of :any:`trust-anchors`
with the ``initial-key`` keyword.

:any:`trusted-keys` Block Grammar
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
.. namedconf:statement:: trusted-keys
   :tags: deprecated

:any:`trusted-keys` Block Definition and Usage
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The :any:`trusted-keys` statement has been deprecated in favor of
:any:`trust-anchors` with the ``static-key`` keyword.

:any:`view` Block Grammar
~~~~~~~~~~~~~~~~~~~~~~~~~~
.. namedconf:statement:: view
   :tags: view
   :short: Allows a name server to answer a DNS query differently depending on who is asking.

::

   view view_name [ class ] {
       match-clients { address_match_list } ;
       match-destinations { address_match_list } ;
       match-recursive-only <boolean> ;
     [ view_option ; ... ]
     [ zone_statement ; ... ]
   } ;

:any:`view` Block Definition and Usage
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The :any:`view` statement is a powerful feature of BIND 9 that lets a name
server answer a DNS query differently depending on who is asking. It is
particularly useful for implementing split DNS setups without having to
run multiple servers.

.. namedconf:statement:: match-clients
   :tags: view
   :short: Specifies a view of DNS namespace for a given subset of client IP addresses.

.. namedconf:statement:: match-destinations
   :tags: view
   :short: Specifies a view of DNS namespace for a given subset of destination IP addresses.

   Each :any:`view` statement defines a view of the DNS namespace that is
   seen by a subset of clients. A client matches a view if its source IP
   address matches the :term:`address_match_list` of the view's
   :any:`match-clients` clause, and its destination IP address matches the
   :term:`address_match_list` of the view's :any:`match-destinations` clause. If
   not specified, both :any:`match-clients` and :any:`match-destinations` default
   to matching all addresses. In addition to checking IP addresses,
   :any:`match-clients` and :any:`match-destinations` can also take the name of a
   TSIG :namedconf:ref:`key`, which provides a mechanism for the client to select
   the view.

.. namedconf:statement:: match-recursive-only
   :tags: view
   :short: Specifies that only recursive requests can match this view of the DNS namespace.

   A view can
   also be specified as :any:`match-recursive-only`, which means that only
   recursive requests from matching clients match that view. The order
   of the :any:`view` statements is significant; a client request is
   resolved in the context of the first :any:`view` that it matches.

Zones defined within a :any:`view` statement are only accessible to
clients that match the :any:`view`. By defining a zone of the same name in
multiple views, different zone data can be given to different clients:
for example, "internal" and "external" clients in a split DNS setup.

Many of the options given in the :namedconf:ref:`options` statement can also be used
within a :any:`view` statement, and then apply only when resolving queries
with that view. When no view-specific value is given, the value in the
:namedconf:ref:`options` statement is used as a default. Also, zone options can have
default values specified in the :any:`view` statement; these view-specific
defaults take precedence over those in the :namedconf:ref:`options` statement.

Views are class-specific. If no class is given, class IN is assumed.
Note that all non-IN views must contain a hint zone, since only the IN
class has compiled-in default hints.

If there are no :any:`view` statements in the config file, a default view
that matches any client is automatically created in class IN. Any
:any:`zone` statements specified on the top level of the configuration file
are considered to be part of this default view, and the :namedconf:ref:`options`
statement applies to the default view. If any explicit :any:`view`
statements are present, all :any:`zone` statements must occur inside
:any:`view` statements.

Here is an example of a typical split DNS setup implemented using
:any:`view` statements:

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

:any:`zone` Block Grammar
~~~~~~~~~~~~~~~~~~~~~~~~~~
.. namedconf:statement:: zone
   :tags: zone
   :short: Specifies the zone in a BIND 9 configuration.
   :suppress_grammar:

:any:`zone` Block Definition and Usage
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Zone Types
^^^^^^^^^^
.. namedconf:statement:: type
   :tags: zone
   :short: Specifies the kind of zone in a given configuration.
   :suppress_grammar:

   The :any:`type` keyword is required for the :any:`zone` configuration unless
   it is an :any:`in-view` configuration. Its acceptable values are:
   :any:`primary <type primary>` (or ``master``), :any:`secondary <type
   secondary>` (or ``slave``), :any:`mirror <type mirror>`, :any:`hint <type
   hint>`, :any:`stub <type stub>`, :any:`static-stub <type static-stub>`,
   :any:`forward <type forward>`, or :any:`redirect <type redirect>`.

.. namedconf:statement:: type primary
   :tags: zone
   :short: Contains the main copy of the data for a zone.

   A primary zone has a master copy of the data for the zone and is able
   to provide authoritative answers for it. Type ``master`` is a synonym
   for :any:`primary <type primary>`.

.. namedconf:statement:: type secondary
   :tags: zone
   :short: Contains a duplicate of the data for a zone that has been transferred from a primary server.

   A secondary zone is a replica of a primary zone. Type ``slave`` is a
   synonym for :any:`secondary <type secondary>`. The :any:`primaries` list
   specifies one or more IP addresses of primary servers that the secondary
   contacts to update its copy of the zone.

   A zone may refresh on timer or on receipt of a notify. If a valid notify is
   received where the notify carries a serial number larger than the one in the
   SOA currently served, then the secondary will schedule a zone refresh.

   A notify is considered valid if the sender is one of the servers in the NS
   RRset for the zone, has been explicitly allowed using an :any:`allow-notify`
   clause, or is from an address listed in the primary servers clause.

   If no notifies have been received, the server will try to refresh the zone.
   The REFRESH field in the SOA record determines how long after the last zone
   update it should query the primaries for the SOA record. Again, if the
   SOA record contains a serial number larger than the one in the SOA currently
   served, a zone refresh is scheduled. If a notify is received while a
   refresh is in progress, the serial number of the notify is checked and if
   it is larger, another refresh for the zone is queued. There will at most
   be one zone refresh queued.

   The primary servers are queried in turn, :any:`named` will move on to the
   next server in the list if either it is unable to get a valid response from
   the server it is currently querying, or the primary being queried returns
   the same or smaller SOA than the secondary is currently serving. On the
   first SOA received that has a serial bigger than the one currently served,
   :any:`named` will initiate a zone transfer with that server. Once the zone
   transfer has been received and the zone has been updated, then this zone
   refresh is complete, and no other servers are tried.

   When receiving a notify, :any:`named` does not first query the sender of
   the notify. It will continue with the next server in the list that
   transferred the zone, skipping over unreachable servers. A primary is
   considered unreachable if the secondary cannot get a response from the
   server. This state will be cached for 10 minutes, or until a notify is
   received from that address.

   Furthermore, a zone is refreshed when the secondary server is restarted,
   or when a :option:`rndc refresh <rndc refresh>` command is received.

   If a file is specified, then the replica is written to this file whenever the zone
   is changed, and reloaded from this file on a server restart. Use of a file
   is recommended, since it often speeds server startup and eliminates a
   needless waste of bandwidth. Note that for large numbers (in the tens or
   hundreds of thousands) of zones per server, it is best to use a two-level
   naming scheme for zone filenames. For example, a secondary server for the
   zone ``example.com`` might place the zone contents into a file called
   ``ex/example.com``, where ``ex/`` is just the first two letters of the zone
   name. (Most operating systems behave very slowly if there are 100,000 files
   in a single directory.)

.. namedconf:statement:: type mirror
   :tags: zone
   :short: Contains a DNSSEC-validated duplicate of the main data for a zone.

   A mirror zone is similar to a zone of :any:`type secondary`, except its
   data is subject to DNSSEC validation before being used in answers.
   Validation is applied to the entire zone during the zone transfer
   process, and again when the zone file is loaded from disk upon
   restarting :iscman:`named`. If validation of a new version of a mirror zone
   fails, a retransfer is scheduled; in the meantime, the most recent
   correctly validated version of that zone is used until it either
   expires or a newer version validates correctly. If no usable zone
   data is available for a mirror zone, due to either transfer failure
   or expiration, traditional DNS recursion is used to look up the
   answers instead. Mirror zones cannot be used in a view that does not
   have recursion enabled.

   Answers coming from a mirror zone look almost exactly like answers
   from a zone of :any:`type secondary`, with the notable exceptions that
   the AA bit ("authoritative answer") is not set, and the AD bit
   ("authenticated data") is.

   Mirror zones are intended to be used to set up a fast local copy of
   the root zone (see :rfc:`8806`). A default list of primary servers
   for the IANA root zone is built into :iscman:`named`, so its mirroring can
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

   To make mirror zone contents persist between :iscman:`named` restarts, use
   the :any:`file` option.

   Mirroring a zone other than root requires an explicit list of primary
   servers to be provided using the :any:`primaries` option (see
   :any:`primaries` for details), and a key-signing key (KSK)
   for the specified zone to be explicitly configured as a trust anchor
   (see :any:`trust-anchors`).

   When configuring NOTIFY for a mirror zone, only ``notify no;`` and
   ``notify explicit;`` can be used at the zone level; any other
   :namedconf:ref:`notify` setting at the zone level is a configuration error. Using
   any other :namedconf:ref:`notify` setting at the :namedconf:ref:`options` or :any:`view` level
   causes that setting to be overridden with ``notify explicit;`` for
   the mirror zone. The global default for the :namedconf:ref:`notify` option is
   ``yes``, so mirror zones are by default configured with ``notify
   explicit;``.

   Outgoing transfers of mirror zones are disabled by default but may be
   enabled using :any:`allow-transfer`.

   .. note::
      Use of this zone type with any zone other than the root should be
      considered *experimental* and may cause performance issues,
      especially for zones that are large and/or frequently updated.

.. namedconf:statement:: type hint
   :tags: zone
   :short: Contains the initial set of root name servers to be used at BIND 9 startup.

   The initial set of root name servers is specified using a hint zone.
   When the server starts, it uses the root hints to find a root name
   server and get the most recent list of root name servers. If no hint zone
   is specified for class IN, the server uses a compiled-in default set of
   root servers hints. Classes other than IN have no built-in default hints.

.. namedconf:statement:: type stub
   :tags: zone
   :short: Contains a duplicate of the NS records of a zone.

   A stub zone specifies a set of name servers to use when contacting the zone
   for the first time.  A stub zone overrides any NS records (delegations)
   that might exist in the parent zone.  Once an authoritative server is
   reached, the NS records from that server are honored.

   A stub zone can work around missing or broken delegations, but comes at
   expense of maintaining a set of name server addresses in
   :iscman:`named.conf`.

   .. warning:: Use of stub zones is not recommended. Proper delegation
                with NS records in the parent zone should be used.


   :iscman:`named` queries authoritative servers configured as :any:`primaries`
   to obtain up-to-date NS records. These new NS records are then used
   to obtain answers from a given zone.

   Stub zones can also be used as a way to force the resolution of a given
   domain to use a particular set of authoritative servers. For example, the
   caching name servers on a private network using :rfc:`1918` addressing may be
   configured with stub zones for ``10.in-addr.arpa`` to use a set of
   internal name servers as the authoritative servers for that domain.

   If a BIND 9 primary, serving a parent zone, has child stub zones configured,
   all the secondary servers for the parent zone also need to have the same
   child stub zones configured.

   Stub zones are not a standard part of the DNS; they are a feature specific
   to the BIND implementation.


.. namedconf:statement:: type static-stub
   :tags: zone
   :short: Contains statically configured NS records for a zone.

   A static-stub zone specifies a set of name servers to use to resolve *all*
   queries for the given zone.  A stub zone overrides any NS records
   (delegations) that might exist in the parent zone, and also any records
   received from the otherwise-authoritative server.

   Like a :any:`stub <type stub>` zone, this can work around missing or broken
   delegations at the parent.  Unlike stub, static-stub also overrides any NS
   records offered by the specified servers.

   When recursion is necessary for a query that matches a static-stub zone,
   the locally configured data (name server names and glue addresses) is
   always used, even if different authoritative information is cached.

   The zone data is configured via the :any:`server-addresses` and
   :any:`server-names` zone statements. These must point to authoritative
   servers.

   .. warning:: Use of static-stub zones is not recommended. Proper delegation
                with NS records in the parent zone should be used.

   The zone data is maintained in the form of NS and (if necessary) glue A or
   AAAA RRs internally, which can be seen by dumping zone databases with
   :option:`rndc dumpdb -all <rndc dumpdb>`. The configured RRs are considered local configuration
   parameters rather than public data. Non-recursive queries (i.e., those
   with the RD bit off) to a static-stub zone are therefore prohibited and
   are responded to with REFUSED.

   Since the data is statically configured, no zone maintenance action takes
   place for a static-stub zone. For example, there is no periodic refresh
   attempt, and an incoming :ref:`NOTIFY <notify>` message is rejected with an rcode
   of NOTAUTH.

   Each static-stub zone is configured with internally generated NS and (if
   necessary) glue A or AAAA RRs.

.. namedconf:statement:: type forward
   :tags: zone
   :short: Contains forwarding statements that apply to queries within a given domain.

   A forward zone is a way to configure forwarding on a per-domain basis.
   A :any:`zone` statement of type :any:`forward` can contain a :any:`forward` and/or
   :any:`forwarders` statement, which applies to queries within the domain
   given by the zone name. If no :any:`forwarders` statement is present, or an
   empty list for :any:`forwarders` is given, then no forwarding is done
   for the domain, canceling the effects of any forwarders in the :namedconf:ref:`options`
   statement. Thus, to use this type of zone to change the
   behavior of the global :any:`forward` option (that is, "forward first" to,
   then "forward only", or vice versa), but use the same servers as set
   globally, re-specify the global forwarders.

.. namedconf:statement:: type redirect
   :tags: zone
   :short: Contains information to answer queries when normal resolution would return NXDOMAIN.

   Redirect zones are used to provide answers to queries when normal
   resolution would result in NXDOMAIN being returned. Only one redirect zone
   is supported per view. :any:`allow-query` can be used to restrict which
   clients see these answers.

   If the client has requested DNSSEC records (DO=1) and the NXDOMAIN response
   is signed, no substitution occurs.

   To redirect all NXDOMAIN responses to 100.100.100.2 and
   2001:ffff:ffff::100.100.100.2, configure a type :any:`redirect <type redirect>` zone
   named ".", with the zone file containing wildcard records that point to
   the desired addresses: ``*. IN A 100.100.100.2`` and
   ``*. IN AAAA 2001:ffff:ffff::100.100.100.2``.

   As another example, to redirect all Spanish names (under .ES), use similar entries
   but with the names ``*.ES.`` instead of ``*.``. To redirect all commercial
   Spanish names (under COM.ES), use wildcard entries
   called ``*.COM.ES.``.

   Note that the redirect zone supports all possible types; it is not
   limited to A and AAAA records.

   If a redirect zone is configured with a :any:`primaries` option, then it is
   transferred in as if it were a secondary zone. Otherwise, it is loaded from a
   file as if it were a primary zone.

   Because redirect zones are not referenced directly by name, they are not
   kept in the zone lookup table with normal primary and secondary zones. To reload
   a redirect zone, use :option:`rndc reload -redirect <rndc reload>`; to retransfer a
   redirect zone configured as a secondary, use :option:`rndc retransfer -redirect <rndc retransfer>`.
   When using :option:`rndc reload` without specifying a zone name, redirect
   zones are reloaded along with other zones.

.. namedconf:statement:: in-view
   :tags: view, zone
   :short: Specifies the view in which a given zone is defined.

   When using multiple views, a :any:`type primary` or :any:`type secondary` zone configured
   in one view can be referenced in a subsequent view. This allows both views
   to use the same zone without the overhead of loading it more than once. This
   is configured using a :any:`zone` statement, with an :any:`in-view` option
   specifying the view in which the zone is defined. A :any:`zone` statement
   containing :any:`in-view` does not need to specify a type, since that is part
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

:any:`allow-notify`
   See the description of :any:`allow-notify` in :ref:`access_control`.

:any:`allow-query`
   See the description of :any:`allow-query` in :ref:`access_control`.

:any:`allow-query-on`
   See the description of :any:`allow-query-on` in :ref:`access_control`.

:any:`allow-transfer`
   See the description of :any:`allow-transfer` in :ref:`access_control`.

:any:`allow-update`
   See the description of :any:`allow-update` in :ref:`access_control`.

:any:`update-policy`
   This specifies a "Simple Secure Update" policy. See :ref:`dynamic_update_policies`.

:any:`allow-update-forwarding`
   See the description of :any:`allow-update-forwarding` in :ref:`access_control`.

:any:`also-notify`
   This option is only meaningful if :namedconf:ref:`notify` is active for this zone. The set of
   machines that receive a ``DNS NOTIFY`` message for this zone is
   made up of all the listed name servers (other than the primary)
   for the zone, plus any IP addresses specified with
   :any:`also-notify`. A port may be specified with each :any:`also-notify`
   address to send the notify messages to a port other than the default
   of 53. A TSIG key may also be specified to cause the ``NOTIFY`` to be
   signed by the given key. :any:`also-notify` is not meaningful for stub
   zones. The default is the empty list.

:any:`check-names`
   This option is used to restrict the character set and syntax of
   certain domain names in primary files and/or DNS responses received
   from the network. The default varies according to zone type. For
   :any:`primary <type primary>` zones the default is ``fail``; for :any:`secondary <type secondary>` zones the
   default is ``warn``. It is not implemented for :any:`hint <type hint>` zones.

:any:`check-mx`
   See the description of :any:`check-mx` in :ref:`boolean_options`.

:any:`check-spf`
   See the description of :any:`check-spf` in :ref:`boolean_options`.

:any:`check-wildcard`
   See the description of :any:`check-wildcard` in :ref:`boolean_options`.

:any:`check-integrity`
   See the description of :any:`check-integrity` in :ref:`boolean_options`.

:any:`check-sibling`
   See the description of :any:`check-sibling` in :ref:`boolean_options`.

:any:`zero-no-soa-ttl`
   See the description of :any:`zero-no-soa-ttl` in :ref:`boolean_options`.

:any:`update-check-ksk`
   See the description of :any:`update-check-ksk` in :ref:`boolean_options`.

:any:`dnssec-loadkeys-interval`
   See the description of :any:`dnssec-loadkeys-interval` in :namedconf:ref:`options`.

:any:`dnssec-update-mode`
   See the description of :any:`dnssec-update-mode` in :namedconf:ref:`options`.

:any:`dnssec-dnskey-kskonly`
   See the description of :any:`dnssec-dnskey-kskonly` in :ref:`boolean_options`.

:any:`try-tcp-refresh`
   See the description of :any:`try-tcp-refresh` in :ref:`boolean_options`.

.. namedconf:statement:: database
   :tags: zone
   :short: Specifies the type of database to be used to store zone data.

   This specifies the type of database to be used to store the zone data.
   The string following the :any:`database` keyword is interpreted as a
   list of whitespace-delimited words. The first word identifies the
   database type, and any subsequent words are passed as arguments to
   the database to be interpreted in a way specific to the database
   type.

   The default is ``rbt``, BIND 9's native in-memory red-black tree
   database. This database does not take arguments.

   Other values are possible if additional database drivers have been
   linked into the server. Some sample drivers are included with the
   distribution but none are linked in by default.

:any:`dialup`
   See the description of :any:`dialup` in :ref:`boolean_options`.

.. namedconf:statement:: file
   :tags: zone
   :short: Specifies the zone's filename.

   This sets the zone's filename. In :any:`primary <type primary>`, :any:`hint <type hint>`, and :any:`redirect <type redirect>`
   zones which do not have :any:`primaries` defined, zone data is loaded from
   this file. In :any:`secondary <type secondary>`, :any:`mirror <type mirror>`, :any:`stub <type stub>`, and :any:`redirect <type redirect>` zones
   which do have :any:`primaries` defined, zone data is retrieved from
   another server and saved in this file. This option is not applicable
   to other zone types.

:any:`forward`
   This option is only meaningful if the zone has a forwarders list. The ``only`` value
   causes the lookup to fail after trying the forwarders and getting no
   answer, while ``first`` allows a normal lookup to be tried.

:any:`forwarders`
   This is used to override the list of global forwarders. If it is not
   specified in a zone of type :any:`forward`, no forwarding is done for
   the zone and the global options are not used.

.. namedconf:statement:: journal
   :tags: zone
   :short: Allows the default journal's filename to be overridden.

   This allows the default journal's filename to be overridden. The default is
   the zone's filename with "``.jnl``" appended. This is applicable to
   :any:`primary <type primary>` and :any:`secondary <type secondary>` zones.

:any:`max-ixfr-ratio`
   See the description of :any:`max-ixfr-ratio` in :namedconf:ref:`options`.

:any:`max-journal-size`
   See the description of :any:`max-journal-size` in :ref:`server_resource_limits`.

:any:`max-records`
   See the description of :any:`max-records` in :ref:`server_resource_limits`.

:any:`min-transfer-rate-in`
   See the description of :any:`min-transfer-rate-in` in :ref:`zone_transfers`.

:any:`max-transfer-time-in`
   See the description of :any:`max-transfer-time-in` in :ref:`zone_transfers`.

:any:`max-transfer-idle-in`
   See the description of :any:`max-transfer-idle-in` in :ref:`zone_transfers`.

:any:`max-transfer-time-out`
   See the description of :any:`max-transfer-time-out` in :ref:`zone_transfers`.

:any:`max-transfer-idle-out`
   See the description of :any:`max-transfer-idle-out` in :ref:`zone_transfers`.

:namedconf:ref:`notify`
   See the description of :namedconf:ref:`notify` in :ref:`boolean_options`.

:any:`notify-delay`
   See the description of :any:`notify-delay` in :ref:`tuning`.

:any:`notify-to-soa`
   See the description of :any:`notify-to-soa` in :ref:`boolean_options`.

:any:`parental-agents`
   This option is only meaningful if the zone is DNSSEC signed. When performing
   a key rollover, BIND will query the parental agents to see if the new DS is
   actually published before withdrawing the old DNSSEC key.

:any:`primaries`
   For secondary zones, these are the name servers to request zone transfers
   from.

:any:`zone-statistics`
   See the description of :any:`zone-statistics` in :namedconf:ref:`options`.

.. namedconf:statement:: server-addresses
   :tags: query, zone
   :short: Specifies a list of IP addresses to which queries should be sent in recursive resolution for a static-stub zone.

   This option is only meaningful for :any:`static-stub <type static-stub>` zones. This is a list of IP addresses
   to which queries should be sent in recursive resolution for the zone.
   A non-empty list for this option internally configures the apex
   NS RR with associated glue A or AAAA RRs.

   For example, if "example.com" is configured as a static-stub zone
   with 192.0.2.1 and 2001:db8::1234 in a :any:`server-addresses` option,
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

.. namedconf:statement:: server-names
   :tags: zone
   :short: Specifies a list of domain names of name servers that act as authoritative servers of a static-stub zone.

   This option is only meaningful for :any:`static-stub <type static-stub>` zones. This is a list of domain names
   of name servers that act as authoritative servers of the static-stub
   zone. These names are resolved to IP addresses when :iscman:`named`
   needs to send queries to these servers. For this supplemental
   resolution to be successful, these names must not be a subdomain of the
   origin name of the static-stub zone. That is, when "example.net" is the
   origin of a static-stub zone, "ns.example" and "master.example.com"
   can be specified in the :any:`server-names` option, but "ns.example.net"
   cannot; it is rejected by the configuration parser.

   A non-empty list for this option internally configures the apex
   NS RR with the specified names. For example, if "example.com" is
   configured as a static-stub zone with "ns1.example.net" and
   "ns2.example.net" in a :any:`server-names` option, the following RRs
   are internally configured:

   ::

      example.com. NS ns1.example.net.
      example.com. NS ns2.example.net.

   These records are used internally to resolve names under the
   static-stub zone. For instance, if the server receives a query for
   "www.example.com" with the RD bit on, the server initiates recursive
   resolution, resolves "ns1.example.net" and/or "ns2.example.net" to IP
   addresses, and then sends queries to one or more of these addresses.

:any:`sig-validity-interval`
   See the description of :any:`sig-validity-interval` in :ref:`tuning`.

:any:`sig-signing-nodes`
   See the description of :any:`sig-signing-nodes` in :ref:`tuning`.

:any:`sig-signing-signatures`
   See the description of :any:`sig-signing-signatures` in
   :ref:`tuning`.

:any:`sig-signing-type`
   See the description of :any:`sig-signing-type` in :ref:`tuning`.

:any:`transfer-source`
   See the description of :any:`transfer-source` in :ref:`zone_transfers`.

:any:`transfer-source-v6`
   See the description of :any:`transfer-source-v6` in :ref:`zone_transfers`.

:any:`notify-source`
   See the description of :any:`notify-source` in :ref:`zone_transfers`.

:any:`notify-source-v6`
   See the description of :any:`notify-source-v6` in :ref:`zone_transfers`.

:any:`min-refresh-time`; :any:`max-refresh-time`; :any:`min-retry-time`; :any:`max-retry-time`
   See the descriptions in :ref:`tuning`.

:any:`ixfr-from-differences`
   See the description of :any:`ixfr-from-differences` in :ref:`boolean_options`.
   (Note that the :any:`ixfr-from-differences` choices of :any:`primary <type primary>` and :any:`secondary <type secondary>`
   are not available at the zone level.)

:any:`key-directory`
   See the description of :any:`key-directory` in :namedconf:ref:`options`.

:any:`serial-update-method`
   See the description of :any:`serial-update-method` in :namedconf:ref:`options`.

.. namedconf:statement:: inline-signing
   :tags: dnssec, zone
   :short: Specifies whether BIND 9 maintains a separate signed version of a zone.

   The use of inline signing is determined by the :any:`dnssec-policy` for
   the zone. If :any:`inline-signing` is explicitly set to ``yes`` or ``no``
   in :any:`zone`, it overrides any value from :any:`dnssec-policy`.

:any:`multi-master`
   See the description of :any:`multi-master` in :ref:`boolean_options`.

:any:`masterfile-format`
   See the description of :any:`masterfile-format` in :ref:`tuning`.

:any:`max-zone-ttl`
   See the description of :any:`max-zone-ttl` in :namedconf:ref:`options`.
   The use of this option in :any:`zone` blocks is deprecated and
   will be rendered non-operational in a future release.

.. _dynamic_update_policies:

Dynamic Update Policies
^^^^^^^^^^^^^^^^^^^^^^^

BIND 9 supports two methods of granting clients the right to
perform dynamic updates to a zone:

- :namedconf:ref:`allow-update` - a simple access control list
- :namedconf:ref:`update-policy` - fine-grained access control

In both cases, BIND 9 writes the updates to the zone's filename
set in :any:`file`.

In the case of a DNSSEC zone where :any:`inline-signing` is disabled, DNSSEC
records are also written to the zone's filename.

   .. note:: The zone file can no longer be manually updated while :iscman:`named`
      is running; it is now necessary to perform :option:`rndc freeze`, edit,
      and then perform :option:`rndc thaw`. Comments and formatting
      in the zone file are lost when dynamic updates occur.

.. namedconf:statement:: update-policy
   :tags: transfer
   :short: Sets fine-grained rules to allow or deny dynamic updates (DDNS), based on requester identity, updated content, etc.

   The :any:`update-policy` clause allows more fine-grained control over which
   updates are allowed. It specifies a set of rules, in which each rule
   either grants or denies permission for one or more names in the zone to
   be updated by one or more identities. Identity is determined by the key
   that signed the update request, using either TSIG or SIG(0). In most
   cases, :any:`update-policy` rules only apply to key-based identities. There
   is no way to specify update permissions based on the client source address.

   :any:`update-policy` rules are only meaningful for zones of
   :any:`type primary`, and are not allowed in any other zone type. It is a
   configuration error to specify both :any:`allow-update` and
   :any:`update-policy` at the same time.

   A pre-defined :any:`update-policy` rule can be switched on with the command
   ``update-policy local;``. :iscman:`named` automatically
   generates a TSIG session key when starting and stores it in a file;
   this key can then be used by local clients to update the zone while
   :iscman:`named` is running. By default, the session key is stored in the file
   |session_key|, the key name is "local-ddns", and the
   key algorithm is HMAC-SHA256. These values are configurable with the
   :any:`session-keyfile`, :any:`session-keyname`, and :any:`session-keyalg` options,
   respectively. A client running on the local system, if run with
   appropriate permissions, may read the session key from the key file and
   use it to sign update requests. The zone's update policy is set to
   allow that key to change any record within the zone. Assuming the key
   name is "local-ddns", this policy is equivalent to:

   ::

      update-policy { grant local-ddns zonesub any; };

   with the additional restriction that only clients connecting from the
   local system are permitted to send updates.

   Note that only one session key is generated by :iscman:`named`; all zones
   configured to use ``update-policy local`` accept the same key.

   The command ``nsupdate -l`` implements this feature, sending requests to
   localhost and signing them using the key retrieved from the session key
   file.

   Other rule definitions look like this:

   ::

      ( grant | deny ) identity ruletype  name   types

   Each rule grants or denies privileges. Rules are checked in the order in
   which they are specified in the :any:`update-policy` statement. Once a
   message has successfully matched a rule, the operation is immediately
   granted or denied, and no further rules are examined. There are 16 types
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

   If the type is immediately followed by a number in parentheses,
   that number is the maximum number of records of that type permitted
   to exist in the RRset after an update has been applied.  For example,
   ``PTR(1)`` indicates that only one PTR record is allowed. If an
   attempt is made to add two PTR records in an update, the second one
   is silently discarded. If a PTR record already exists, both
   new records are silently discarded.

   If type ANY is specified with a limit, then that limit applies to
   all types that are not otherwise specified.  For example, ``A PTR(1)
   ANY(2)`` indicates that an unlimited number of A records can exist,
   but only one PTR record, and no more than two of any other type.

   Typical use with a rule ``grant * tcp-self . PTR(1);`` in the zone
   2.0.192.IN-ADDR.ARPA looks like this:

   ::

     nsupdate -v <<EOF
     local 192.0.2.1
     del 1.2.0.192.IN-ADDR.ARPA PTR
     add 1.2.0.192.IN-ADDR.ARPA 0 PTR EXAMPLE.COM
     send
     EOF

   The ruletype field has 18 values: ``name``, ``subdomain``, ``zonesub``,
   ``wildcard``, ``self``, ``selfsub``, ``selfwild``, ``ms-self``,
   ``ms-selfsub``, ``ms-subdomain``, ``ms-subdomain-self-rhs``, ``krb5-self``,
   ``krb5-selfsub``, ``krb5-subdomain``,  ``krb5-subdomain-self-rhs``,
   ``tcp-self``, ``6to4-self``, and ``external``.

   ``name``
       With exact-match semantics, this rule matches when the name being updated is identical to the contents of the ``name`` field.

   ``subdomain``
       This rule matches when the name being updated is a subdomain of, or identical to, the contents of the ``name`` field.

   ``zonesub``
       This rule is similar to subdomain, except that it matches when the name being updated is a subdomain of the zone in which the :any:`update-policy` statement appears. This obviates the need to type the zone name twice, and enables the use of a standard :any:`update-policy` statement in multiple zones without modification.
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

       For example, if :any:`update-policy` for the zone "example.com" includes ``grant EXAMPLE.COM ms-subdomain hosts.example.com. AA AAAA``, any machine with a valid principal in the realm ``EXAMPLE.COM`` is able to update address records at or below ``hosts.example.com``.

   ``ms-subdomain-self-rhs``
       This rule is similar to ``ms-subdomain``, with an additional
       restriction that PTR and SRV target names must match the name of the
       machine identified in the principal.

   ``krb5-self``
       When a client sends an UPDATE using a Kerberos machine principal (for example, ``host/machine@REALM``), this rule allows records with the absolute name of ``machine`` to be updated, provided it has been authenticated by REALM. This is similar but not identical to ``ms-self``, due to the ``machine`` part of the Kerberos principal being an absolute name instead of an unqualified name.

       The realm to be matched is specified in the ``identity`` field.

       The ``name`` field has no effect on this rule; it should be set to "." as a placeholder.

       For example, ``grant EXAMPLE.COM krb5-self . A AAAA`` allows any machine with a valid principal in the realm ``EXAMPLE.COM`` to update its own address records.

   ``krb5-selfsub``
       This is similar to ``krb5-self``, except it also allows updates to any subdomain of the name specified in the ``machine`` part of the Kerberos principal, not just to the name itself.

   ``krb5-subdomain``
       This rule is identical to ``ms-subdomain``, except that it works with Kerberos machine principals (i.e., ``host/machine@REALM``) rather than Windows machine principals.

   ``krb5-subdomain-self-rhs``
       This rule is similar to ``krb5-subdomain``, with an additional
       restriction that PTR and SRV target names must match the name of the
       machine identified in the principal.

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
       This rule allows :iscman:`named` to defer the decision of whether to allow a given update to an external daemon.

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
contain identical zones. The :any:`in-view` zone option provides an
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

An :any:`in-view` option cannot refer to a view that is configured later in
the configuration file.

A :any:`zone` statement which uses the :any:`in-view` option may not use any
other options, with the exception of :any:`forward` and :any:`forwarders`.
(These options control the behavior of the containing view, rather than
change the zone object itself.)

Zone-level ACLs (e.g., allow-query, allow-transfer), and other
configuration details of the zone, are all set in the view the referenced
zone is defined in. Be careful to ensure that ACLs are wide
enough for all views referencing the zone.

An :any:`in-view` zone cannot be used as a response policy zone.

An :any:`in-view` zone is not intended to reference a :any:`forward` zone.

Statements
----------
BIND 9 supports many hundreds of statements; finding the right statement to
control a specific behavior or solve a particular problem can be a daunting
task. To simplify the task for users, all statements have been assigned one or more tags.
Tags are designed to group together statements that have broadly similar
functionality; thus, for example, all statements that control the handling of
queries or of zone transfers are respectively tagged under **query** and **transfer**.

:ref:`dnssec_tag_statements` are those that relate to or control DNSSEC.

:ref:`logging_tag_statements` relate to or control logging, and typically only
appear in a logging block.

:ref:`query_tag_statements` relate to or control queries.

:ref:`security_tag_statements` relate to or control security features.

:ref:`server_tag_statements` relate to or control server behavior, and typically
only appear in a server block.

:ref:`transfer_tag_statements` relate to or control zone transfers.

:ref:`view_tag_statements` relate to or control view selection criteria, and
typically only appear in a view block.

:ref:`zone_tag_statements` relate to or control zone behavior, and typically
only appear in a zone block.

:ref:`deprecated_tag_statements` are those that are now deprecated, but are
included here for historical reference.

The following table lists all statements permissible in :file:`named.conf`, with their
associated tags; the next section groups the statements by tag. Please note that these
sections are a work in progress.

.. namedconf:statementlist::

Statements by Tag
-----------------
These tables group the various statements permissible in :file:`named.conf` by
their corresponding tag.

.. _dnssec_tag_statements:

DNSSEC Tag Statements
~~~~~~~~~~~~~~~~~~~~~
.. namedconf:statementlist::
   :filter_tags: dnssec

.. _logging_tag_statements:

Logging Tag Statements
~~~~~~~~~~~~~~~~~~~~~~
.. namedconf:statementlist::
   :filter_tags: logging

.. _query_tag_statements:

Query Tag Statements
~~~~~~~~~~~~~~~~~~~~
.. namedconf:statementlist::
   :filter_tags: query

.. _security_tag_statements:

Security Tag Statements
~~~~~~~~~~~~~~~~~~~~~~~
.. namedconf:statementlist::
   :filter_tags: security

.. _server_tag_statements:

Server Tag Statements
~~~~~~~~~~~~~~~~~~~~~
.. namedconf:statementlist::
   :filter_tags: server

.. _transfer_tag_statements:

Transfer Tag Statements
~~~~~~~~~~~~~~~~~~~~~~~
.. namedconf:statementlist::
   :filter_tags: transfer

.. _view_tag_statements:

View Tag Statements
~~~~~~~~~~~~~~~~~~~
.. namedconf:statementlist::
   :filter_tags: view

.. _zone_tag_statements:

Zone Tag Statements
~~~~~~~~~~~~~~~~~~~
.. namedconf:statementlist::
   :filter_tags: zone

.. _deprecated_tag_statements:

Deprecated Tag Statements
~~~~~~~~~~~~~~~~~~~~~~~~~
.. namedconf:statementlist::
   :filter_tags: deprecated

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

Incoming Zone Transfers
   Information about in-progress incoming zone transfers.

   This section describes the information that can be seen in the
   HTML table about in-progress incoming zone transfers. It lists
   the meaning, units, and possible range of values of each column,
   and the key/attribute/element name (in parentheses) for the JSON
   and XML output formats.

   ``Zone Name`` (``name``)
      Text string. This is the name of the zone being transferred,
      as specified in the :any:`zone` declaration on this server.

   ``Zone Type`` (``type``)
      Text string. This is the type of zone being transferred, as
      specified in the ``zone`` declaration on this server. Possible
      values are: ``secondary``, ``stub``, ``redirect``, and ``mirror``.

   ``Local Serial`` (``serial``)
      32-bit unsigned Integer. This is the current (old) serial
      number of the zone being transferred. It comes from the SOA
      record held on the current server.

   ``Remote Serial`` (``remoteserial``)
      32-bit unsigned Integer. This is the new serial number of the
      zone being transferred. It comes from the SOA record held on
      the primary server from which the zone is being transferred.

   ``IXFR`` (``ixfr``)
      Boolean. This says whether the transfer is incremental (using
      IXFR) or full (using AXFR). Possible values are: ``Yes`` and
      ``No``.

   ``State`` (``state``)
      Text string. This is the current state of the transfer for
      this zone. Possible values and their meanings are:

         ``Needs Refresh``
	     The zone needs a refresh, but the process has not started yet;
	     this can be due to different factors, like the retry interval of
	     the zone.

         ``Pending``
	     The zone is flagged for a refresh, but the process is currently
	     in the queue and will start shortly, or is in a waiting state
	     because of rate-limiting; see :any:`serial-query-rate`. The
	     ``Duration (s)`` timer starts before entering this state.

         ``Refresh SOA``
	     BIND is sending a refresh SOA query to get the zone serial number and will then
	     initiate a zone transfer, if necessary. If this step is successful,
	     the ``SOA Query`` and ``Got SOA`` states are skipped.
	     Otherwise, the zone transfer procedure can still be initiated,
	     and the SOA request will be attempted using the same transport as
	     the zone transfer. The ``Duration (s)`` timer restarts before
	     entering this state, and for each attempted connection (note that
	     in UDP mode there can be several retries during one "connection"
	     attempt).

         ``Deferred``
	     The zone is going to be refreshed, but the process was
	     deferred due to quota; see :any:`transfers-in` and
	     :any:`transfers-per-ns`. The ``Duration (s)`` timer restarts before
	     entering this state.

         ``SOA Query``
	     BIND is sending an SOA query to get the zone serial number and will then
	     follow with a zone transfer, if necessary. The ``Duration (s)``
	     timer restarts before entering this state.

         ``Got SOA``
	     An answer for the SOA query from the previous step is
	     received, initiating a transfer.

         ``Zone Transfer Request``
	     BIND is waiting for the zone transfer to start. The ``Duration (s)`` timer
	     restarts before entering this state.

         ``First Data``
	     BIND is waiting for the first data record of the transfer.

         ``Receiving IXFR Data``
	     BIND is receiving data for an IXFR type incremental zone
	     transfer.

         ``Finalizing IXFR``
             BIND is finalizing an IXFR type incremental zone transfer.

         ``Receiving AXFR Data``
             BIND is receiving data for an AXFR type zone transfer.

         ``Finalizing AXFR``
             BIND is finalizing an AXFR type zone transfer.

      .. note::
         State names can change between BIND versions.

   ``Additional Refresh Queued`` (``refreshqueued``)
      Boolean. This shows that the zone is flagged for a refresh.
      This can be set to ``Yes`` either when the zone transfer is
      still in one of the pending states (see the description of
      the ``State`` column), or when the transfer is in a running
      state, but the zone was marked for another refresh again (e.g.
      because of "notify" request from a primary server). Possible
      values are: ``Yes`` and ``No``.

   ``Local Address`` (``localaddr``)
      IP address (IPv4 or IPv6, as appropriate) and port number.
      This shows the source address used to establish the connection
      for the transfer.

   ``Remote Address`` (``remoteaddr``)
      IP address (IPv4 or IPv6, as appropriate) and port number.
      This shows the destination address used to establish the
      connection for the transfer.

   ``SOA Transport`` (``soatransport``)
      Text string. This is the transport protocol in use for the
      SOA query.  Note that this value can potentially change during the
      process. For example, when the transfer is in the ``Refresh SOA``
      state, the ``SOA Transport`` of the ongoing query can be shown as ``UDP``.
      If that query fails or times out, it then can be retried using another
      transport, or the transfer process can be initiated in "SOA before" mode,
      where the SOA query will be attempted using the same transport as the zone
      transfer. See the description of the ``State`` field for more information.
      Possible values are: ``UDP``, ``TCP``, ``TLS``, and ``None``.

   ``Transport`` (``transport``)
      Text string. This is the transport protocol in use for the
      transfer. Possible values are: ``TCP`` and ``TLS``.

   ``TSIG Key Name`` (``tsigkeyname``)
      Text string. This is the name of the TSIG key specified for
      use with this zone in the :any:`zone` declaration (if any).

   ``Duration (s)`` (``duration``)
      64-bit unsigned Integer. This is the time, in seconds, that
      the current major state of the transfer process has been running so far.
      The timer starts after the refresh SOA request is queued (before the
      ``Pending`` state), and then restarts several times during the
      process to indicate the duration of the current major state. See the
      descriptions of the different states to find out the states before which
      this timer restarts.

   ``Messages Received`` (``nmsg``)
      64-bit unsigned Integer. This is the number of DNS messages
      received. It does not include transport overheads, such as
      TCP ACK.

   ``Records Received`` (``nrecs``)
      64-bit unsigned Integer. This is the number of individual RRs
      received so far. If an address record has, for example, five
      addresses associated with the same name, it counts as five
      RRs.

   ``Bytes Received`` (``nbytes``)
      64-bit unsigned Integer. This is the number of usable bytes
      of DNS data. It does not include transport overhead.

   ``Transfer Rate (B/s)`` (``rate``)
      64 bit unsigned Integer. This is the average zone transfer rate in
      bytes-per-second during the latest full interval that is configured by the
      :any:`min-transfer-rate-in` configuration option. If no such interval
      has passed yet, then the overall average rate is reported instead.

   .. note::
      Depending on the current state of the transfer, some of the
      values may be empty or set to ``-`` (meaning "not available").
      Also, in the case of the JSON output format, the corresponding
      keys can be missing or values can be set to ``NULL``.  For
      example, it is unknown whether a transfer is using AXFR or
      IXFR until the first data is received (see the description
      of the ``State`` column).

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
   may only be used if stale answers are enabled. If an RR type name is
   preceded by a tilde (~), it represents the number of RRsets for this type
   that are present in the cache database but are marked for garbage collection;
   these RRsets cannot be used.

Socket I/O Statistics
   Statistics counters for network-related events.

A subset of Name Server Statistics is collected and shown per zone for
which the server has the authority, when :any:`zone-statistics` is set to
``full`` (or ``yes``), for backward compatibility. See the description of
:any:`zone-statistics` in :namedconf:ref:`options` for further details.

These statistics counters are shown with their zone and view names. The
view name is omitted when the server is not configured with explicit
views.

There are currently two user interfaces to get access to the statistics.
One is in plain-text format, dumped to the file specified by the
:any:`statistics-file` configuration option; the other is remotely
accessible via a statistics channel when the :any:`statistics-channels`
statement is specified in the configuration file.

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
    This indicates the number of queries that caused the server to perform recursion in order to find the final answer. This corresponds to the :any:`recursion` counter of previous versions of BIND 9.

``QryDuplicate``
    This indicates the number of queries which the server attempted to recurse but for which it discovered an existing query with the same IP address, port, query ID, name, type, and class already being processed. This corresponds to the ``duplicate`` counter of previous versions of BIND 9.

``QryDropped``
    This indicates the number of recursive queries dropped by the server as a result of configured limits. These limits include the settings of the :any:`fetches-per-zone`, :any:`fetches-per-server`, :any:`clients-per-query`, and :any:`max-clients-per-query` options, as well as the :any:`rate-limit` option. This corresponds to the ``dropped`` counter of previous versions of BIND 9.

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
    requests exceeded :any:`update-quota`.

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
    This indicates the number of mismatched responses received, meaning the DNS ID, response's source address, and/or the response's source port does not match what was expected. (The port must be 53 or as defined by the :namedconf:ref:`port` option.) This may be an indication of a cache poisoning attempt.

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

``QueryCurUDP``
    This indicates the number of UDP queries in progress.

``QueryCurTCP``
    This indicates the number of TCP queries in progress.

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

``NumFetch``
    This indicates the number of active fetches.

``BucketSize``
    This indicates the number of the resolver's internal buckets (a static number).

``REFUSED``
    This indicates the number of REFUSED responses received.

``ClientCookieOut``
    This indicates the number of COOKIE messages sent to an authoritative server with only a client cookie.

``ServerCookieOut``
    This indicates the number of COOKIE messages sent to an authoritative server with both a client and a cached server cookie.

``CookieIn``
    This indicates the number of COOKIE replies received from an authoritative server.

``CookieClientOk``
    This indicates the number of correctly formed COOKIE client responses received.

``BadEDNSVersion``
    This indicates the number of bad EDNS version replies received.

``BadCookieRcode``
    This indicates the number of BADCOOKIE response codes received from an authoritative server.

``ZoneQuota``
    This indicates the number of queries spilled for exceeding the :any:`fetches-per-zone` quota.

``ServerQuota``
    This indicates the number of queries spilled for exceeding the :any:`fetches-per-server` quota.

``ClientQuota``
    This indicates the number of queries spilled for exceeding the :any:`clients-per-query` quota.

``NextItem``
    This indicates the number of times the server waited for the next item after receiving an invalid response.

``Priming``
    This indicates the number of priming fetches performed by the resolver.

.. _socket_stats:

Socket I/O Statistics Counters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Socket I/O statistics counters are defined per socket type, which are
``UDP4`` (UDP/IPv4), ``UDP6`` (UDP/IPv6), ``TCP4`` (TCP/IPv4), and ``TCP6``
(TCP/IPv6). In the following list, ``<TYPE>`` represents
a socket type. Not all counters are available for all socket types;
exceptions are noted in the descriptions.

``<TYPE>Open``
    This indicates the number of sockets opened successfully.

``<TYPE>OpenFail``
    This indicates the number of failures to open sockets.

``<TYPE>Close``
    This indicates the number of closed sockets.

``<TYPE>BindFail``
    This indicates the number of failures to bind sockets.

``<TYPE>ConnFail``
    This indicates the number of failures to connect sockets.

``<TYPE>Conn``
    This indicates the number of connections established successfully.

``<TYPE>AcceptFail``
    This indicates the number of failures to accept incoming connection requests. This counter does not apply to the ``UDP`` type.

``<TYPE>Accept``
    This indicates the number of incoming connections successfully accepted. This counter does not apply to the ``UDP`` type.

``<TYPE>SendErr``
    This indicates the number of errors in socket send operations.

``<TYPE>RecvErr``
    This indicates the number of errors in socket receive operations, including errors of send operations on a connected UDP socket, notified by an ICMP error message.
