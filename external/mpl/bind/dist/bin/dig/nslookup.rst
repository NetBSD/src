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

.. highlight: console

.. iscman:: nslookup
.. program:: nslookup
.. _man_nslookup:

nslookup - query Internet name servers interactively
----------------------------------------------------

Synopsis
~~~~~~~~

:program:`nslookup` [-option] [name | -] [server]

Description
~~~~~~~~~~~

:program:`nslookup` is a program to query Internet domain name servers.
:program:`nslookup` has two modes: interactive and non-interactive. Interactive
mode allows the user to query name servers for information about various
hosts and domains or to print a list of hosts in a domain.
Non-interactive mode prints just the name and requested
information for a host or domain.

Arguments
~~~~~~~~~

Interactive mode is entered in the following cases:

a. when no arguments are given (the default name server is used);

b. when the first argument is a hyphen (-) and the second argument is
   the host name or Internet address of a name server.

Non-interactive mode is used when the name or Internet address of the
host to be looked up is given as the first argument. The optional second
argument specifies the host name or address of a name server.

Options can also be specified on the command line if they precede the
arguments and are prefixed with a hyphen. For example, to change the
default query type to host information, with an initial timeout of 10
seconds, type:

::

   nslookup -query=hinfo  -timeout=10

The ``-version`` option causes :program:`nslookup` to print the version number
and immediately exit.

Interactive Commands
~~~~~~~~~~~~~~~~~~~~

``host [server]``
   This command looks up information for :iscman:`host` using the current default server or
   using ``server``, if specified. If :iscman:`host` is an Internet address and the
   query type is A or PTR, the name of the host is returned. If :iscman:`host` is
   a name and does not have a trailing period (``.``), the search list is used
   to qualify the name.

   To look up a host not in the current domain, append a period to the
   name.

``server domain`` | ``lserver domain``
   These commands change the default server to ``domain``; ``lserver`` uses the initial
   server to look up information about ``domain``, while ``server`` uses the
   current default server. If an authoritative answer cannot be found,
   the names of servers that might have the answer are returned.

``root``
   This command is not implemented.

``finger``
   This command is not implemented.

``ls``
   This command is not implemented.

``view``
   This command is not implemented.

``help``
   This command is not implemented.

``?``
   This command is not implemented.

``exit``
   This command exits the program.

``set keyword[=value]``
   This command is used to change state information that affects the
   lookups. Valid keywords are:

   ``all``
      This keyword prints the current values of the frequently used options to
      ``set``. Information about the current default server and host is
      also printed.

   ``class=value``
      This keyword changes the query class to one of:

      ``IN``
         the Internet class

      ``CH``
         the Chaos class

      ``HS``
         the Hesiod class

      ``ANY``
         wildcard

      The class specifies the protocol group of the information. The default
      is ``IN``; the abbreviation for this keyword is ``cl``.

   ``nodebug``
      This keyword turns on or off the display of the full response packet, and any
      intermediate response packets, when searching. The default for this keyword is
      ``nodebug``; the abbreviation for this keyword is ``[no]deb``.

   ``nod2``
      This keyword turns debugging mode on or off. This displays more about what
      nslookup is doing. The default is ``nod2``.

   ``domain=name``
      This keyword sets the search list to ``name``.

   ``nosearch``
      If the lookup request contains at least one period, but does not end
      with a trailing period, this keyword appends the domain names in the domain
      search list to the request until an answer is received. The default is ``search``.

   ``port=value``
      This keyword changes the default TCP/UDP name server port to ``value`` from
      its default, port 53. The abbreviation for this keyword is ``po``.

   ``querytype=value`` | ``type=value``
      This keyword changes the type of the information query to ``value``. The
      defaults are A and then AAAA; the abbreviations for these keywords are
      ``q`` and ``ty``.

      Please note that it is only possible to specify one query type. Only the default
      behavior looks up both when an alternative is not specified.

   ``norecurse``
      This keyword tells the name server to query other servers if it does not have
      the information. The default is ``recurse``; the abbreviation for this
      keyword is ``[no]rec``.

   ``ndots=number``
      This keyword sets the number of dots (label separators) in a domain that
      disables searching. Absolute names always stop searching.

   ``retry=number``
      This keyword sets the number of retries to ``number``.

   ``timeout=number``
      This keyword changes the initial timeout interval to wait for a reply to
      ``number``, in seconds.

   ``novc``
      This keyword indicates that a virtual circuit should always be used when sending requests to the server.
      ``novc`` is the default.

   ``nofail``
      This keyword tries the next nameserver if a nameserver responds with SERVFAIL or
      a referral (nofail), or terminates the query (fail) on such a response. The
      default is ``nofail``.

Return Values
~~~~~~~~~~~~~

:program:`nslookup` returns with an exit status of 1 if any query failed, and 0
otherwise.

IDN Support
~~~~~~~~~~~

If :program:`nslookup` has been built with IDN (internationalized domain name)
support, it can accept and display non-ASCII domain names. :program:`nslookup`
appropriately converts character encoding of a domain name before sending
a request to a DNS server or displaying a reply from the server.
To turn off IDN support, define the ``IDN_DISABLE``
environment variable. IDN support is disabled if the variable is set
when :program:`nslookup` runs, or when the standard output is not a tty.

Files
~~~~~

``/etc/resolv.conf``

See Also
~~~~~~~~

:iscman:`dig(1) <dig>`, :iscman:`host(1) <host>`, :iscman:`named(8) <named>`.
