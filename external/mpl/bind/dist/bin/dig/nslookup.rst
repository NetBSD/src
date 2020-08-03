.. 
   Copyright (C) Internet Systems Consortium, Inc. ("ISC")
   
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/.
   
   See the COPYRIGHT file distributed with this work for additional
   information regarding copyright ownership.

..
   Copyright (C) Internet Systems Consortium, Inc. ("ISC")

   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/.

   See the COPYRIGHT file distributed with this work for additional
   information regarding copyright ownership.


.. highlight: console

.. _man_nslookup:

nslookup - query Internet name servers interactively
----------------------------------------------------

Synopsis
~~~~~~~~

:program:`nslookup` [-option] [name | -] [server]

Description
~~~~~~~~~~~

``Nslookup`` is a program to query Internet domain name servers.
``Nslookup`` has two modes: interactive and non-interactive. Interactive
mode allows the user to query name servers for information about various
hosts and domains or to print a list of hosts in a domain.
Non-interactive mode is used to print just the name and requested
information for a host or domain.

Arguments
~~~~~~~~~

Interactive mode is entered in the following cases:

a. when no arguments are given (the default name server will be used)

b. when the first argument is a hyphen (-) and the second argument is
   the host name or Internet address of a name server.

Non-interactive mode is used when the name or Internet address of the
host to be looked up is given as the first argument. The optional second
argument specifies the host name or address of a name server.

Options can also be specified on the command line if they precede the
arguments and are prefixed with a hyphen. For example, to change the
default query type to host information, and the initial timeout to 10
seconds, type:

::

   nslookup -query=hinfo  -timeout=10

The ``-version`` option causes ``nslookup`` to print the version number
and immediately exits.

Interactive Commands
~~~~~~~~~~~~~~~~~~~~

``host`` [server]
   Look up information for host using the current default server or
   using server, if specified. If host is an Internet address and the
   query type is A or PTR, the name of the host is returned. If host is
   a name and does not have a trailing period, the search list is used
   to qualify the name.

   To look up a host not in the current domain, append a period to the
   name.

``server`` domain | ``lserver`` domain
   Change the default server to domain; ``lserver`` uses the initial
   server to look up information about domain, while ``server`` uses the
   current default server. If an authoritative answer can't be found,
   the names of servers that might have the answer are returned.

``root``
   not implemented

``finger``
   not implemented

``ls``
   not implemented

``view``
   not implemented

``help``
   not implemented

``?``
   not implemented

``exit``
   Exits the program.

``set`` keyword[=value]
   This command is used to change state information that affects the
   lookups. Valid keywords are:

   ``all``
      Prints the current values of the frequently used options to
      ``set``. Information about the current default server and host is
      also printed.

   ``class=``\ value
      Change the query class to one of:

      ``IN``
         the Internet class

      ``CH``
         the Chaos class

      ``HS``
         the Hesiod class

      ``ANY``
         wildcard

      The class specifies the protocol group of the information.

      (Default = IN; abbreviation = cl)

   ``nodebug``
      Turn on or off the display of the full response packet and any
      intermediate response packets when searching.

      (Default = nodebug; abbreviation = [no]deb)

   ``nod2``
      Turn debugging mode on or off. This displays more about what
      nslookup is doing.

      (Default = nod2)

   ``domain=``\ name
      Sets the search list to name.

   ``nosearch``
      If the lookup request contains at least one period but doesn't end
      with a trailing period, append the domain names in the domain
      search list to the request until an answer is received.

      (Default = search)

   ``port=``\ value
      Change the default TCP/UDP name server port to value.

      (Default = 53; abbreviation = po)

   ``querytype=``\ value | ``type=``\ value
      Change the type of the information query.

      (Default = A and then AAAA; abbreviations = q, ty)

      **Note:** It is only possible to specify one query type, only the default
        behavior looks up both when an alternative is not specified.

   ``norecurse``
      Tell the name server to query other servers if it does not have
      the information.

      (Default = recurse; abbreviation = [no]rec)

   ``ndots=``\ number
      Set the number of dots (label separators) in a domain that will
      disable searching. Absolute names always stop searching.

   ``retry=``\ number
      Set the number of retries to number.

   ``timeout=``\ number
      Change the initial timeout interval for waiting for a reply to
      number seconds.

   ``novc``
      Always use a virtual circuit when sending requests to the server.

      (Default = novc)

   ``nofail``
      Try the next nameserver if a nameserver responds with SERVFAIL or
      a referral (nofail) or terminate query (fail) on such a response.

      (Default = nofail)

Return Values
~~~~~~~~~~~~~

``nslookup`` returns with an exit status of 1 if any query failed, and 0
otherwise.

IDN Support
~~~~~~~~~~~

If ``nslookup`` has been built with IDN (internationalized domain name)
support, it can accept and display non-ASCII domain names. ``nslookup``
appropriately converts character encoding of domain name before sending
a request to DNS server or displaying a reply from the server. If you'd
like to turn off the IDN support for some reason, define the IDN_DISABLE
environment variable. The IDN support is disabled if the variable is set
when ``nslookup`` runs or when the standard output is not a tty.

Files
~~~~~

``/etc/resolv.conf``

See Also
~~~~~~~~

:manpage:`dig(1)`, :manpage:`host(1)`, :manpage:`named(8)`.
