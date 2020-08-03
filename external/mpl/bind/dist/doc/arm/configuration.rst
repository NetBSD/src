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

.. Configuration:

Name Server Configuration
=========================

In this chapter we provide some suggested configurations along with
guidelines for their use. We suggest reasonable values for certain
option settings.

.. _sample_configuration:

Sample Configurations
---------------------

.. _cache_only_sample:

A Caching-only Name Server
~~~~~~~~~~~~~~~~~~~~~~~~~~

The following sample configuration is appropriate for a caching-only
name server for use by clients internal to a corporation. All queries
from outside clients are refused using the ``allow-query`` option.
Alternatively, the same effect could be achieved using suitable firewall
rules.

::

   // Two corporate subnets we wish to allow queries from.
   acl corpnets { 192.168.4.0/24; 192.168.7.0/24; };
   options {
        // Working directory
        directory "/etc/namedb";

        allow-query { corpnets; };
   };
   // Provide a reverse mapping for the loopback
   // address 127.0.0.1
   zone "0.0.127.in-addr.arpa" {
        type master;
        file "localhost.rev";
        notify no;
   };

.. _auth_only_sample:

An Authoritative-only Name Server
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This sample configuration is for an authoritative-only server that is
the primary (master) server for ``example.com`` and a secondary (slave) server for the subdomain
``eng.example.com``.

::

   options {
        // Working directory
        directory "/etc/namedb";
        // Do not allow access to cache
        allow-query-cache { none; };
        // This is the default
        allow-query { any; };
        // Do not provide recursive service
        recursion no;
   };

   // Provide a reverse mapping for the loopback
   // address 127.0.0.1
   zone "0.0.127.in-addr.arpa" {
        type master;
        file "localhost.rev";
        notify no;
   };
   // We are the master server for example.com
   zone "example.com" {
        type master;
        file "example.com.db";
        // IP addresses of slave servers allowed to
        // transfer example.com
        allow-transfer {
         192.168.4.14;
         192.168.5.53;
        };
   };
   // We are a slave server for eng.example.com
   zone "eng.example.com" {
        type slave;
        file "eng.example.com.bk";
        // IP address of eng.example.com master server
        masters { 192.168.4.12; };
   };

.. _load_balancing:

Load Balancing
--------------

A primitive form of load balancing can be achieved in the DNS by using
multiple records (such as multiple A records) for one name.

For example, assuming three HTTP servers with network addresses of
10.0.0.1, 10.0.0.2 and 10.0.0.3, a set of records such as the following
means that clients will connect to each machine one third of the time:

+-----------+------+----------+----------+----------------------------+
| Name      | TTL  | CLASS    | TYPE     | Resource Record (RR) Data  |
+-----------+------+----------+----------+----------------------------+
| www       | 600  |   IN     |   A      |   10.0.0.1                 |
+-----------+------+----------+----------+----------------------------+
|           | 600  |   IN     |   A      |   10.0.0.2                 |
+-----------+------+----------+----------+----------------------------+
|           | 600  |   IN     |   A      |   10.0.0.3                 |
+-----------+------+----------+----------+----------------------------+

When a resolver queries for these records, BIND rotates them and
responds to the query with the records in a different order. In the
example above, clients randomly receive records in the order 1, 2,
3; 2, 3, 1; and 3, 1, 2. Most clients use the first record returned
and discard the rest.

For more detail on ordering responses, check the ``rrset-order``
sub-statement in the ``options`` statement; see :ref:`rrset_ordering`.

.. _ns_operations:

Name Server Operations
----------------------

.. _tools:

Tools for Use With the Name Server Daemon
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This section describes several indispensable diagnostic, administrative, 
and monitoring tools available to the system administrator for
controlling and debugging the name server daemon.

.. _diagnostic_tools:

Diagnostic Tools
^^^^^^^^^^^^^^^^

The ``dig``, ``host``, and ``nslookup`` programs are all command-line
tools for manually querying name servers. They differ in style and
output format.

``dig``
   ``dig`` is the most versatile and complete of these lookup tools. It
   has two modes: simple interactive mode for a single query, and batch
   mode which executes a query for each in a list of several query
   lines. All query options are accessible from the command line.

   ``dig [@server] domain [query-type][query-class][+query-option][-dig-option][%comment]``

   The usual simple use of ``dig`` will take the form

   ``dig @server domain query-type query-class``

   For more information and a list of available commands and options,
   see the ``dig`` man page.

``host``
   The ``host`` utility emphasizes simplicity and ease of use. By
   default, it converts between host names and Internet addresses, but
   its functionality can be extended with the use of options.

   ``host [-aCdlnrsTwv][-c class][-N ndots][-t type][-W timeout][-R retries][-m flag][-4][-6] hostname [server]``

   For more information and a list of available commands and options,
   see the ``host`` man page.

``nslookup``
   ``nslookup`` has two modes: interactive and non-interactive.
   Interactive mode allows the user to query name servers for
   information about various hosts and domains or to print a list of
   hosts in a domain. Non-interactive mode is used to print just the
   name and requested information for a host or domain.

   ``nslookup [-option][ [host-to-find]|[-[server]] ]``

   Interactive mode is entered when no arguments are given (the default
   name server is used) or when the first argument is a hyphen
   (``-``) and the second argument is the host name or Internet address of
   a name server.

   Non-interactive mode is used when the name or Internet address of the
   host to be looked up is given as the first argument. The optional
   second argument specifies the host name or address of a name server.

   Due to its arcane user interface and frequently inconsistent
   behavior, we do not recommend the use of ``nslookup``. Use ``dig``
   instead.

.. _admin_tools:

Administrative Tools
^^^^^^^^^^^^^^^^^^^^

Administrative tools play an integral part in the management of a
server.

``named-checkconf``
   The ``named-checkconf`` program checks the syntax of a ``named.conf``
   file.

   ``named-checkconf [-jvz][-t directory][filename]``

``named-checkzone``
   The ``named-checkzone`` program checks a master file for syntax and
   consistency.

   ``named-checkzone [-djqvD][-c class][-o output][-t directory][-w directory][-k (ignore|warn|fail)][-n (ignore|warn|fail)][-W (ignore|warn)] zone [filename]``

``named-compilezone``
   This tool is similar to ``named-checkzone,`` but it always dumps the zone content
   to a specified file (typically in a different format).

``rndc``
   The remote name daemon control (``rndc``) program allows the system
   administrator to control the operation of a name server. If ``rndc`` is run
   without any options, it will display a usage message as
   follows:

   ``rndc [-c config][-s server][-p port][-y key] command [command...]``

   See :ref:`man_rndc` for details of the available ``rndc``
   commands.

   ``rndc`` requires a configuration file, since all communication with
   the server is authenticated with digital signatures that rely on a
   shared secret, and there is no way to provide that secret other than
   with a configuration file. The default location for the ``rndc``
   configuration file is ``/etc/rndc.conf``, but an alternate location
   can be specified with the ``-c`` option. If the configuration file is
   not found, ``rndc`` will also look in ``/etc/rndc.key`` (or whatever
   ``sysconfdir`` was defined when the BIND build was configured). The
   ``rndc.key`` file is generated by running ``rndc-confgen -a`` as
   described in :ref:`controls_statement_definition_and_usage`.

   The format of the configuration file is similar to that of
   ``named.conf``, but is limited to only four statements: the ``options``,
   ``key``, ``server``, and ``include`` statements. These statements are
   what associate the secret keys to the servers with which they are
   meant to be shared. The order of statements is not significant.

   The ``options`` statement has three clauses: ``default-server``,
   ``default-key``, and ``default-port``. ``default-server`` takes a
   host name or address argument and represents the server that will be
   contacted if no ``-s`` option is provided on the command line.
   ``default-key`` takes the name of a key as its argument, as defined
   by a ``key`` statement. ``default-port`` specifies the port to which
   ``rndc`` should connect if no port is given on the command line or in
   a ``server`` statement.

   The ``key`` statement defines a key to be used by ``rndc`` when
   authenticating with ``named``. Its syntax is identical to the ``key``
   statement in ``named.conf``. The keyword ``key`` is followed by a key
   name, which must be a valid domain name, though it need not actually
   be hierarchical; thus, a string like "``rndc_key``" is a valid name.
   The ``key`` statement has two clauses: ``algorithm`` and ``secret``.
   While the configuration parser will accept any string as the argument
   to the algorithm, currently only the strings ``hmac-md5``,
   ``hmac-sha1``, ``hmac-sha224``, ``hmac-sha256``,
   ``hmac-sha384``, and ``hmac-sha512`` have any meaning. The secret
   is a Base64 encoded string as specified in :rfc:`3548`.

   The ``server`` statement associates a key defined using the ``key``
   statement with a server. The keyword ``server`` is followed by a host
   name or address. The ``server`` statement has two clauses: ``key``
   and ``port``. The ``key`` clause specifies the name of the key to be
   used when communicating with this server, and the ``port`` clause can
   be used to specify the port ``rndc`` should connect to on the server.

   A sample minimal configuration file is as follows:

   ::

      key rndc_key {
           algorithm "hmac-sha256";
           secret
             "c3Ryb25nIGVub3VnaCBmb3IgYSBtYW4gYnV0IG1hZGUgZm9yIGEgd29tYW4K";
      };
      options {
           default-server 127.0.0.1;
           default-key    rndc_key;
      };

   This file, if installed as ``/etc/rndc.conf``, allows the
   command:

   ``$ rndc reload``

   to connect to 127.0.0.1 port 953 and cause the name server to reload,
   if a name server on the local machine is running with the following
   controls statements:

   ::

      controls {
          inet 127.0.0.1
              allow { localhost; } keys { rndc_key; };
      };

   and it has an identical key statement for ``rndc_key``.

   Running the ``rndc-confgen`` program conveniently creates a
   ``rndc.conf`` file, and also displays the corresponding
   ``controls`` statement needed to add to ``named.conf``.
   Alternatively, it is possible to run ``rndc-confgen -a`` to set up a
   ``rndc.key`` file and not modify ``named.conf`` at all.

Signals
~~~~~~~

Certain UNIX signals cause the name server to take specific actions, as
described in the following table. These signals can be sent using the
``kill`` command.

+--------------+-------------------------------------------------------+
| ``SIGHUP``   | Causes the server to read ``named.conf`` and reload   |
|              | the database.                                         |
+--------------+-------------------------------------------------------+
| ``SIGTERM``  | Causes the server to clean up and exit.               |
+--------------+-------------------------------------------------------+
| ``SIGINT``   | Causes the server to clean up and exit.               |
+--------------+-------------------------------------------------------+

.. include:: plugins.rst
