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

.. _man_named:

named - Internet domain name server
-----------------------------------

Synopsis
~~~~~~~~

:program:`named` [ [**-4**] | [**-6**] ] [**-c** config-file] [**-d** debug-level] [**-D** string] [**-E** engine-name] [**-f**] [**-g**] [**-L** logfile] [**-M** option] [**-m** flag] [**-n** #cpus] [**-p** port] [**-s**] [**-S** #max-socks] [**-t** directory] [**-U** #listeners] [**-u** user] [**-v**] [**-V**] [**-X** lock-file] [**-x** cache-file]

Description
~~~~~~~~~~~

``named`` is a Domain Name System (DNS) server, part of the BIND 9
distribution from ISC. For more information on the DNS, see :rfc:`1033`,
:rfc:`1034`, and :rfc:`1035`.

When invoked without arguments, ``named`` will read the default
configuration file ``/etc/named.conf``, read any initial data, and
listen for queries.

Options
~~~~~~~

**-4**
   Use IPv4 only even if the host machine is capable of IPv6. ``-4`` and
   ``-6`` are mutually exclusive.

**-6**
   Use IPv6 only even if the host machine is capable of IPv4. ``-4`` and
   ``-6`` are mutually exclusive.

**-c** config-file
   Use config-file as the configuration file instead of the default,
   ``/etc/named.conf``. To ensure that reloading the configuration file
   continues to work after the server has changed its working directory
   due to to a possible ``directory`` option in the configuration file,
   config-file should be an absolute pathname.

**-d** debug-level
   Set the daemon's debug level to debug-level. Debugging traces from
   ``named`` become more verbose as the debug level increases.

**-D** string
   Specifies a string that is used to identify a instance of ``named``
   in a process listing. The contents of string are not examined.

**-E** engine-name
   When applicable, specifies the hardware to use for cryptographic
   operations, such as a secure key store used for signing.

   When BIND is built with OpenSSL PKCS#11 support, this defaults to the
   string "pkcs11", which identifies an OpenSSL engine that can drive a
   cryptographic accelerator or hardware service module. When BIND is
   built with native PKCS#11 cryptography (--enable-native-pkcs11), it
   defaults to the path of the PKCS#11 provider library specified via
   "--with-pkcs11".

**-f**
   Run the server in the foreground (i.e. do not daemonize).

**-g**
   Run the server in the foreground and force all logging to ``stderr``.

**-L** logfile
   Log to the file ``logfile`` by default instead of the system log.

**-M** option
   Sets the default memory context options. If set to external, this
   causes the internal memory manager to be bypassed in favor of
   system-provided memory allocation functions. If set to fill, blocks
   of memory will be filled with tag values when allocated or freed, to
   assist debugging of memory problems. (nofill disables this behavior,
   and is the default unless ``named`` has been compiled with developer
   options.)

**-m** flag
   Turn on memory usage debugging flags. Possible flags are usage,
   trace, record, size, and mctx. These correspond to the
   ISC_MEM_DEBUGXXXX flags described in ``<isc/mem.h>``.

**-n** #cpus
   Create #cpus worker threads to take advantage of multiple CPUs. If
   not specified, ``named`` will try to determine the number of CPUs
   present and create one thread per CPU. If it is unable to determine
   the number of CPUs, a single worker thread will be created.

**-p** port
   Listen for queries on port port. If not specified, the default is
   port 53.

**-s**
   Write memory usage statistics to ``stdout`` on exit.

.. note::

      This option is mainly of interest to BIND 9 developers and may be
      removed or changed in a future release.

**-S** #max-socks
   Allow ``named`` to use up to #max-socks sockets. The default value is
   21000 on systems built with default configuration options, and 4096
   on systems built with "configure --with-tuning=small".

.. warning::

      This option should be unnecessary for the vast majority of users.
      The use of this option could even be harmful because the specified
      value may exceed the limitation of the underlying system API. It
      is therefore set only when the default configuration causes
      exhaustion of file descriptors and the operational environment is
      known to support the specified number of sockets. Note also that
      the actual maximum number is normally a little fewer than the
      specified value because ``named`` reserves some file descriptors
      for its internal use.

**-t** directory
   Chroot to directory after processing the command line arguments, but
   before reading the configuration file.

.. warning::

      This option should be used in conjunction with the ``-u`` option,
      as chrooting a process running as root doesn't enhance security on
      most systems; the way ``chroot(2)`` is defined allows a process
      with root privileges to escape a chroot jail.

**-U** #listeners
   Use #listeners worker threads to listen for incoming UDP packets on
   each address. If not specified, ``named`` will calculate a default
   value based on the number of detected CPUs: 1 for 1 CPU, and the
   number of detected CPUs minus one for machines with more than 1 CPU.
   This cannot be increased to a value higher than the number of CPUs.
   If ``-n`` has been set to a higher value than the number of detected
   CPUs, then ``-U`` may be increased as high as that value, but no
   higher. On Windows, the number of UDP listeners is hardwired to 1 and
   this option has no effect.

**-u** user
   Setuid to user after completing privileged operations, such as
   creating sockets that listen on privileged ports.

.. note::

      On Linux, ``named`` uses the kernel's capability mechanism to drop
      all root privileges except the ability to ``bind(2)`` to a
      privileged port and set process resource limits. Unfortunately,
      this means that the ``-u`` option only works when ``named`` is run
      on kernel 2.2.18 or later, or kernel 2.3.99-pre3 or later, since
      previous kernels did not allow privileges to be retained after
      ``setuid(2)``.

**-v**
   Report the version number and exit.

**-V**
   Report the version number and build options, and exit.

**-X** lock-file
   Acquire a lock on the specified file at runtime; this helps to
   prevent duplicate ``named`` instances from running simultaneously.
   Use of this option overrides the ``lock-file`` option in
   ``named.conf``. If set to ``none``, the lock file check is disabled.

**-x** cache-file
   Load data from cache-file into the cache of the default view.

.. warning::

      This option must not be used. It is only of interest to BIND 9
      developers and may be removed or changed in a future release.

Signals
~~~~~~~

In routine operation, signals should not be used to control the
nameserver; ``rndc`` should be used instead.

SIGHUP
   Force a reload of the server.

SIGINT, SIGTERM
   Shut down the server.

The result of sending any other signals to the server is undefined.

Configuration
~~~~~~~~~~~~~

The ``named`` configuration file is too complex to describe in detail
here. A complete description is provided in the BIND 9 Administrator
Reference Manual.

``named`` inherits the ``umask`` (file creation mode mask) from the
parent process. If files created by ``named``, such as journal files,
need to have custom permissions, the ``umask`` should be set explicitly
in the script used to start the ``named`` process.

Files
~~~~~

``/etc/named.conf``
   The default configuration file.

``/var/run/named/named.pid``
   The default process-id file.

See Also
~~~~~~~~

:rfc:`1033`, :rfc:`1034`, :rfc:`1035`, :manpage:`named-checkconf(8)`, :manpage:`named-checkzone(8)`, :manpage:`rndc(8), :manpage:`named.conf(5)`, BIND 9 Administrator Reference Manual.
