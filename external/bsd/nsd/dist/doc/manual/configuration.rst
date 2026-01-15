Configuration
=============

NSD has a vast array of configuration options for advanced use cases. To
configure the application, a ``nsd.conf`` configuration file used. The file
format has attributes and values, and some attributes have attributes inside
them.

.. Note:: The instructions in this page assume that NSD is already installed.

The configuration file
----------------------


The configuration NSD uses is specified in the configuration file, which can be supplied to NSD using the :option:`-c` option. In the :doc:`reference<manpages/nsd.conf>` an example ``nsd.conf`` can be found as well as the complete documentation of all the configurable options. The same example and reference can be found on your system using the ``man nsd.conf`` command.


The basic rules are of the config file are:

  - The used notation is ``attribute: value``
  - Comments start with ``#`` and extend to the end of a line
  - Empty lines are ignored, as is whitespace at the beginning of a line
  - Quotes can be used, for names containing spaces, e.g. ``"file name.zone"``


Below we'll give an example config file, which specifies options for the NSD server, zone files, primaries and secondaries. This provide basic config which can be used for as a starting point.

Note that for the remainder we assume the default location of NSD is ``/etc/nsd`` though this may vary on your system.

The example configuration below specifies options for the NSD server, zone
files, primaries and secondaries.

Here is an example config for ``example.com``:

.. code:: bash

        server:
            # use this number of cpu cores
            server-count: 1
            #  the default file used for the nsd-control addzone and delzone commands
            # zonelistfile: "/var/db/nsd/zone.list"
            # The unprivileged user that will run NSD, can also be set to "" if
            # user privilige protection is not needed
            username: nsd
            # Default file where all the log messages go
            logfile: "/var/log/nsd.log"
            # Use this pid file instead of the platform specific default
            pidfile: "/var/run/nsd.pid"
            # Enable if privilege "jail" is needed for unprivileged user. Note
            # that other file paths may break when using chroot
            # chroot: "/etc/nsd/"
            # The default zone transfer file
            # xfrdfile: "/var/db/nsd/xfrd.state"
            # The default working directory before accessing zone files
            # zonesdir: "/etc/nsd"

        remote-control:
            # this allows the use of 'nsd-control' to control NSD. The default is "no"
            control-enable: yes
            # the interface NSD listens to for nsd-control. The default is 127.0.0.1 and ::1
            control-interface: 127.0.0.1
            # the key files that allow the use of 'nsd-control'. The default path is "/etc/nsd/". Create these using the 'nsd-control-setup' utility
            server-key-file: /etc/nsd/nsd_server.key
            server-cert-file: /etc/nsd/nsd_server.pem
            control-key-file: /etc/nsd/nsd_control.key
            control-cert-file: /etc/nsd/nsd_control.pem

        zone:
            name: example.com
            zonefile: /etc/nsd/example.com.zone

We recommend keeping the ``server-count`` lower or equal to the number of CPU cores your system has.

Optionally, you can control NSD (from the same or even a different device) by using the entries under the `remote-control` clause in the config. Using this tool, NSD can be controlled (find the reference of all the options :doc:`here<manpages/nsd-control>`) which makes controlling NSD much easier. If your install does not come with the keys needed for remote-control use pre-made, you can generate the keys using the :command:`nsd-control-setup` command, which will create them for you. In the section below we will go into more detail about this option.

You can test the config with :command:`nsd-checkconf`. This tool will tell you what is wrong with the config and where the error occurs.

If you are happy with the config and any modifications you may have done, you can create the zone to go with the file we mentioned in the config. We show an example zone at :doc:`the zonefile example<zonefile>`.


Setting up a secondary zone
---------------------------

If your needs go further than just a few zones that are managed locally, NSD has got you covered. We won't go into the theoretical details of primaries and secondaries here (we recommend `this blog <https://www.cloudflare.com/en-gb/learning/dns/glossary/primary-secondary-dns/>`_), but we will show how to configure it.


The example for a secondary looks like this:

.. code:: bash

        zone:
            # this server is the primary, 192.0.2.1 is the secondary.
            name: primaryzone.com
            zonefile: /etc/nsd/primaryzone.com.zone
            notify: 192.0.2.1 NOKEY # NOKEY for testing purposes only
            provide-xfr: 192.0.2.1 NOKEY # NOKEY for testing purposes only

        zone:
            # this server is secondary, 192.0.2.2 is primary.
            name: secondaryzone.com
            zonefile: /etc/nsd/secondaryzone.com.zone
            allow-notify: 192.0.2.2 NOKEY # NOKEY for testing purposes only
            request-xfr: 192.0.2.2 NOKEY # NOKEY for testing purposes only

.. note::

    Note that the ``NOKEY`` keyword above are for testing purposes only, as this can introduce vulnerabilities when used in production environments.



For a secondary zone we list the primaries by IP address. Below is an example
of a secondary zone with two primary servers. If a primary only supports AXFR
transfers and not IXFR transfers (like NSD), specify the primary as
``request-xfr: AXFR <ip_address> <key>``. By default, all zone transfer requests
are made over TCP. If you want the IXFR request be transmitted over UDP, use
``request-xfr: UDP <ip address> <key>``.

.. code-block:: text

  zone:
    name: "example.com"
    zonefile: "example.com.zone"
    allow-notify: 168.192.185.33 NOKEY
    request-xfr: 168.192.185.33 NOKEY
    allow-notify: 168.192.199.2 NOKEY
    request-xfr: 168.192.199.2 NOKEY

By default, a secondary will fallback to AXFR requests if the primary told us it
does not support IXFR. You can configure the secondary not to do AXFR fallback
with:

.. code-block:: text

    allow-axfr-fallback: "no"

For a primary zone, list the secondary servers, by IP address or subnet. Below
is an example of a primary zone with two secondary servers:

.. code-block:: text

    zone:
        name: "example.com"
        zonefile: "example.com.zone"
        notify: 168.192.133.75 NOKEY
        provide-xfr: 168.192.133.75 NOKEY
        notify: 168.192.5.44 NOKEY
        provide-xfr: 168.192.5.44 NOKEY

You also can set the outgoing interface for notifies and zone transfer requests
to satisfy access control lists at the other end:

.. code-block:: text

    outgoing-interface: 168.192.5.69

By default, NSD will retry a notify up to five times. You can override that
value with:

.. code-block:: text

    notify-retry: 5

Zone transfers can be secured with TSIG keys, replace NOKEY with the name of the
TSIG key to use. See :doc:`Using TSIG<running/using-tsig>` for details.

Since NSD is written to be run on root name servers, the config file can 
contain something like:

.. code-block:: text

    zone:
        name: "."
        zonefile: "root.zone"
        provide-xfr: 0.0.0.0/0 NOKEY # allow axfr for everyone.
        provide-xfr: ::0/0 NOKEY

You should only do that if you're intending to run a root server, NSD is not
suited for running a ``.`` cache. Therefore if you choose to serve the ``.``
zone you have to make sure that the complete root zone is timely and fully
updated.

To prevent misconfiguration, NSD configure has the
``--enable-root-server`` option, that is by default disabled.

In the config file, you can use patterns. A pattern can have the same
configuration statements that a zone can have.  And then you can
``include-pattern: <name-of-pattern>`` in a zone (or in another pattern) to
apply those settings. This can be used to organise the settings.


Remote controlling NSD
----------------------

The :command:`nsd-control` tool is also controlled from the ``nsd.conf`` config
file (and it's manpage is found :doc:`here<manpages/nsd-control>`). It uses TLS encrypted transport to 127.0.0.1, and if you want to use it
you have to setup the keys and also edit the config file.  You can leave the
remote-control disabled (the secure default), or opt to turn it on:

.. code-block:: text

    # generate keys
    nsd-control-setup

.. code-block:: text

  # edit nsd.conf to add this
  remote-control:
    control-enable: yes

By default :command:`nsd-control` is limited to localhost, as well as encrypted,
but some people may want to remotely administer their nameserver.  To control NSD remotely, configure :command:`nsd-control` to listen to the public IP address with
``control-interface: <IP>`` after the control-enable statement.

Furthermore, you copy the key files :file:`/etc/nsd/nsd_server.pem`
:file:`/etc/nsd/nsd_control.*` to a remote host on the internet; on that host
you can run :command:`nsd-control` with :option:`-c` ``<special config file>``
which references same IP address ``control-interface`` and references the copies
of the key files with ``server-cert-file``, ``control-key-file`` and
``control-cert-file`` config lines after the ``control-enable`` statement.  The
nsd-server authenticates the nsd-control client, and also the
:command:`nsd-control` client authenticates the nsd-server.


Starting up the first time
--------------------------

When you are done with the configuration file, check the syntax using

.. code-block:: text

    nsd-checkconf <name of configfile>

You can start the daemon in a number of ways:

.. code-block:: text

    nsd -c <name of configfile>
    nsd-control start # which execs nsd via the remote-control configuration
    nsd # which will use the default configuration file

To check if the daemon is running look with :command:`ps`, :command:`top`, or if
you enabled :command:`nsd-control`:

.. code-block:: text

    nsd-control status

To reload changed zone files after you edited them, without stopping the daemon,
use this to check if files are modified:

.. code-block:: text

    kill -HUP `cat <name of nsd pidfile>`
    or "nsd-control reload" if you have remote-control enabled

With :command:`nsd-control` you can also reread the config file, in case of new
zones, etc.

.. code-block:: text

    nsd-control reconfig

To restart the daemon:

.. code-block:: text

    /etc/rc.d/nsd restart    # or your system(d) equivalent

To shut it down (for example on the system shutdown) do:

.. code-block:: text

    kill -TERM <pid of nsd>
    or nsd-control stop

NSD will automatically keep track of secondary zones and update them when
needed. When primary zones are updated and reloaded notifications are sent to
secondary servers.

To write changed contents of the zone files for secondary zones to disk in the
text-based zone file format, issue :command:`nsd-control write`.

NSD will send notifications to secondary zones if a primary zone is updated. NSD
will check for updates at primary servers periodically and transfer the updated
zone by AXFR/IXFR and reload the new zone contents.

If you wish exert manual control use :command:`nsd-control notify`,
:command:`transfer` and :command:`force_transfer` commands.  The transfer
command will check for new versions of the secondary zones hosted by this NSD.
The notify command will send notifications to the secondary servers configured
in ``notify:`` statements.
