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

.. _man_named-checkconf:

named-checkconf - named configuration file syntax checking tool
---------------------------------------------------------------

Synopsis
~~~~~~~~

:program:`named-checkconf` [**-chjlvz**] [**-p** [**-x** ]] [**-t** directory] {filename}

Description
~~~~~~~~~~~

``named-checkconf`` checks the syntax, but not the semantics, of a
``named`` configuration file. The file, along with all files included by it, is parsed and checked for syntax
errors. If no file is specified,
``/etc/named.conf`` is read by default.

Note: files that ``named`` reads in separate parser contexts, such as
``rndc.key`` and ``bind.keys``, are not automatically read by
``named-checkconf``. Configuration errors in these files may cause
``named`` to fail to run, even if ``named-checkconf`` was successful.
However, ``named-checkconf`` can be run on these files explicitly.

Options
~~~~~~~

``-h``
   This option prints the usage summary and exits.

``-j``
   When loading a zonefile, this option instructs ``named`` to read the journal if it exists.

``-l``
   This option lists all the configured zones. Each line of output contains the zone
   name, class (e.g. IN), view, and type (e.g. primary or secondary).

``-c``
   This option specifies that only the "core" configuration should be checked. This suppresses the loading of
   plugin modules, and causes all parameters to ``plugin`` statements to
   be ignored.

``-i``
   This option ignores warnings on deprecated options.

``-p``
   This option prints out the ``named.conf`` and included files in canonical form if
   no errors were detected. See also the ``-x`` option.

``-t directory``
   This option instructs ``named`` to chroot to ``directory``, so that ``include`` directives in the
   configuration file are processed as if run by a similarly chrooted
   ``named``.

``-v``
   This option prints the version of the ``named-checkconf`` program and exits.

``-x``
   When printing the configuration files in canonical form, this option obscures
   shared secrets by replacing them with strings of question marks
   (``?``). This allows the contents of ``named.conf`` and related files
   to be shared - for example, when submitting bug reports -
   without compromising private data. This option cannot be used without
   ``-p``.

``-z``
   This option performs a test load of all zones of type ``primary`` found in ``named.conf``.

``filename``
   This indicates the name of the configuration file to be checked. If not specified,
   it defaults to ``/etc/named.conf``.

Return Values
~~~~~~~~~~~~~

``named-checkconf`` returns an exit status of 1 if errors were detected
and 0 otherwise.

See Also
~~~~~~~~

:manpage:`named(8)`, :manpage:`named-checkzone(8)`, BIND 9 Administrator Reference Manual.
