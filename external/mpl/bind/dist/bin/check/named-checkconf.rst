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

.. iscman:: named-checkconf
.. program:: named-checkconf
.. _man_named-checkconf:

named-checkconf - named configuration file syntax checking tool
---------------------------------------------------------------

Synopsis
~~~~~~~~

:program:`named-checkconf` [**-achjklvz**] [**-p** [**-x** ]] [**-t** directory] {filename}

Description
~~~~~~~~~~~

:program:`named-checkconf` checks the syntax, but not the semantics, of a
:iscman:`named` configuration file. The file, along with all files included by it, is parsed and checked for syntax
errors. If no file is specified,
|named_conf| is read by default.

Note: files that :iscman:`named` reads in separate parser contexts, such as
``rndc.conf`` or ``rndc.key``, are not automatically read by
:program:`named-checkconf`.  Configuration errors in these files may cause
:iscman:`named` to fail to run, even if :program:`named-checkconf` was
successful.  However, :program:`named-checkconf` can be run on these files
explicitly.

Options
~~~~~~~

.. option:: -a

   Don't check the `dnssec-policy`'s DNSSEC key algorithms against
   those supported by the crypto provider.  This is useful when checking
   a `named.conf` intended to be run on another machine with possibly a
   different set of supported DNSSEC key algorithms.

.. option:: -h

   This option prints the usage summary and exits.

.. option:: -j

   When loading a zonefile, this option instructs :iscman:`named` to read the journal if it exists.

.. option:: -k

   Check the `dnssec-policy`'s DNSSEC keys against the key files in
   the `key-directory`.  This is useful when checking a `named.conf`
   to ensure a DNSSEC policy matches the existing keys.

.. option:: -l

   This option lists all the configured zones. Each line of output contains the zone
   name, class (e.g. IN), view, and type (e.g. primary or secondary).

.. option:: -c

   This option specifies that only the "core" configuration should be checked. This suppresses the loading of
   plugin modules, and causes all parameters to ``plugin`` statements to
   be ignored.

.. option:: -i

   This option ignores warnings on deprecated options.

.. option:: -p

   This option prints out the :iscman:`named.conf` and included files in canonical form if
   no errors were detected. See also the :option:`-x` option.

.. option:: -t directory

   This option instructs :iscman:`named` to chroot to ``directory``, so that ``include`` directives in the
   configuration file are processed as if run by a similarly chrooted
   :iscman:`named`.

.. option:: -v

   This option prints the version of the :program:`named-checkconf` program and exits.

.. option:: -x

   When printing the configuration files in canonical form, this option obscures
   shared secrets by replacing them with strings of question marks
   (``?``). This allows the contents of :iscman:`named.conf` and related files
   to be shared - for example, when submitting bug reports -
   without compromising private data. This option cannot be used without
   :option:`-p`.

.. option:: -z

   This option performs a test load of all zones of type ``primary`` found in :iscman:`named.conf`.

.. option:: filename

   This indicates the name of the configuration file to be checked. If not specified,
   it defaults to |named_conf|.

Return Values
~~~~~~~~~~~~~

:program:`named-checkconf` returns an exit status of 1 if errors were detected
and 0 otherwise.

See Also
~~~~~~~~

:iscman:`named(8) <named>`, :iscman:`named-checkzone(8) <named-checkzone>`, BIND 9 Administrator Reference Manual.
