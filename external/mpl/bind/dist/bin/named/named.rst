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

.. iscman:: named
.. program:: named
.. _man_named:

named - Internet domain name server
-----------------------------------

Synopsis
~~~~~~~~

:program:`named` [ [**-4**] | [**-6**] ] [**-c** config-file] [**-C**] [**-d** debug-level] [**-D** string] [**-E** engine-name] [**-f**] [**-g**] [**-L** logfile] [**-M** option] [**-m** flag] [**-n** #cpus] [**-p** port] [**-s**] [**-t** directory] [**-U** #listeners] [**-u** user] [**-v**] [**-V**] [**-X** lock-file]

Description
~~~~~~~~~~~

:program:`named` is a Domain Name System (DNS) server, part of the BIND 9
distribution from ISC. For more information on the DNS, see :rfc:`1033`,
:rfc:`1034`, and :rfc:`1035`.

When invoked without arguments, :program:`named` reads the default
configuration file |named_conf|, reads any initial data, and
listens for queries.

Options
~~~~~~~

.. option:: -4

   This option tells :program:`named` to use only IPv4, even if the host machine is capable of IPv6. :option:`-4` and
   :option:`-6` are mutually exclusive.

.. option:: -6

   This option tells :program:`named` to use only IPv6, even if the host machine is capable of IPv4. :option:`-4` and
   :option:`-6` are mutually exclusive.

.. option:: -c config-file

   This option tells :program:`named` to use ``config-file`` as its configuration file instead of the default,
   |named_conf|. To ensure that the configuration file
   can be reloaded after the server has changed its working directory
   due to to a possible ``directory`` option in the configuration file,
   ``config-file`` should be an absolute pathname.

.. option:: -C

   This option prints out the default built-in configuration and exits.

   NOTE: This is for debugging purposes only and is not an
   accurate representation of the actual configuration used by :iscman:`named`
   at runtime.

.. option:: -d debug-level

   This option sets the daemon's debug level to ``debug-level``. Debugging traces from
   :program:`named` become more verbose as the debug level increases.

.. option:: -D string

   This option specifies a string that is used to identify a instance of :program:`named`
   in a process listing. The contents of ``string`` are not examined.

.. option:: -E engine-name

   When applicable, this option specifies the hardware to use for cryptographic
   operations, such as a secure key store used for signing.

   When BIND 9 is built with OpenSSL, this needs to be set to the OpenSSL
   engine identifier that drives the cryptographic accelerator or
   hardware service module (usually ``pkcs11``).

.. option:: -f

   This option runs the server in the foreground (i.e., do not daemonize).

.. option:: -g

   This option runs the server in the foreground and forces all logging to ``stderr``.

.. option:: -L logfile

   This option sets the log to the file ``logfile`` by default, instead of the system log.

.. option:: -M option

   This option sets the default (comma-separated) memory context
   options. The possible flags are:

   - ``fill``: fill blocks of memory with tag values when they are
     allocated or freed, to assist debugging of memory problems; this is
     the implicit default if :program:`named` has been compiled with
     ``--enable-developer``.

   - ``nofill``: disable the behavior enabled by ``fill``; this is the
     implicit default unless :program:`named` has been compiled with
     ``--enable-developer``.

.. option:: -m flag

   This option turns on memory usage debugging flags. Possible flags are ``usage``,
   ``trace``, ``record``, ``size``, and ``mctx``. These correspond to the
   ``ISC_MEM_DEBUGXXXX`` flags described in ``<isc/mem.h>``.

.. option:: -n #cpus

   This option creates ``#cpus`` worker threads to take advantage of multiple CPUs. If
   not specified, :program:`named` tries to determine the number of CPUs
   present and creates one thread per CPU. If it is unable to determine
   the number of CPUs, a single worker thread is created.

.. option:: -p value

   This option specifies the port(s) on which the server will listen
   for queries. If ``value`` is of the form ``<portnum>`` or
   ``dns=<portnum>``, the server will listen for DNS queries on
   ``portnum``; if not not specified, the default is port 53. If
   ``value`` is of the form ``tls=<portnum>``, the server will
   listen for TLS queries on ``portnum``; the default is 853.
   If ``value`` is of the form ``https=<portnum>``, the server will
   listen for HTTPS queries on ``portnum``; the default is 443.
   If ``value`` is of the form ``http=<portnum>``, the server will
   listen for HTTP queries on ``portnum``; the default is 80.

.. option:: -s

   This option writes memory usage statistics to ``stdout`` on exit.

.. note::

      This option is mainly of interest to BIND 9 developers and may be
      removed or changed in a future release.

.. option:: -S #max-socks

   This option is deprecated and no longer has any function.

.. warning::

      This option should be unnecessary for the vast majority of users.
      The use of this option could even be harmful, because the specified
      value may exceed the limitation of the underlying system API. It
      is therefore set only when the default configuration causes
      exhaustion of file descriptors and the operational environment is
      known to support the specified number of sockets. Note also that
      the actual maximum number is normally slightly fewer than the
      specified value, because :program:`named` reserves some file descriptors
      for its internal use.

.. option:: -t directory

   This option tells :program:`named` to chroot to ``directory`` after processing the command-line arguments, but
   before reading the configuration file.

.. warning::

      This option should be used in conjunction with the :option:`-u` option,
      as chrooting a process running as root doesn't enhance security on
      most systems; the way ``chroot`` is defined allows a process
      with root privileges to escape a chroot jail.

.. option:: -U #listeners

   This option tells :program:`named` the number of ``#listeners`` worker threads to listen on, for incoming UDP packets on
   each address. If not specified, :program:`named` calculates a default
   value based on the number of detected CPUs: 1 for 1 CPU, and the
   number of detected CPUs minus one for machines with more than 1 CPU.
   This cannot be increased to a value higher than the number of CPUs.
   If :option:`-n` has been set to a higher value than the number of detected
   CPUs, then :option:`-U` may be increased as high as that value, but no
   higher.

.. option:: -u user

   This option sets the setuid to ``user`` after completing privileged operations, such as
   creating sockets that listen on privileged ports.

.. note::

      On Linux, :program:`named` uses the kernel's capability mechanism to drop
      all root privileges except the ability to ``bind`` to a
      privileged port and set process resource limits. Unfortunately,
      this means that the :option:`-u` option only works when :program:`named` is run
      on kernel 2.2.18 or later, or kernel 2.3.99-pre3 or later, since
      previous kernels did not allow privileges to be retained after
      ``setuid``.

.. option:: -v

   This option reports the version number and exits.

.. option:: -V

   This option reports the version number, build options, supported
   cryptographics algorithms, and exits.

.. option:: -X lock-file

   This option acquires a lock on the specified file at runtime; this helps to
   prevent duplicate :program:`named` instances from running simultaneously.
   Use of this option overrides the ``lock-file`` option in
   :iscman:`named.conf`. If set to ``none``, the lock file check is disabled.

Signals
~~~~~~~

In routine operation, signals should not be used to control the
nameserver; :iscman:`rndc` should be used instead.

SIGHUP
   This signal forces a reload of the server.

SIGINT, SIGTERM
   These signals shut down the server.

The result of sending any other signals to the server is undefined.

Configuration
~~~~~~~~~~~~~

The :program:`named` configuration file is too complex to describe in detail
here. A complete description is provided in the BIND 9 Administrator
Reference Manual.

:program:`named` inherits the ``umask`` (file creation mode mask) from the
parent process. If files created by :program:`named`, such as journal files,
need to have custom permissions, the ``umask`` should be set explicitly
in the script used to start the :program:`named` process.

Files
~~~~~

|named_conf|
   The default configuration file.

|named_pid|
   The default process-id file.

See Also
~~~~~~~~

:rfc:`1033`, :rfc:`1034`, :rfc:`1035`, :iscman:`named-checkconf(8) <named-checkconf>`, :iscman:`named-checkzone(8) <named-checkzone>`, :iscman:`rndc(8) <rndc>`, :iscman:`named.conf(5) <named.conf>`, BIND 9 Administrator Reference Manual.
