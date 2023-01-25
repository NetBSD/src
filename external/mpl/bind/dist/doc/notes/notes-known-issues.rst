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

.. _relnotes_known_issues:

Known Issues
------------

- Upgrading from BIND 9.16.32 or any older version may require a manual
  configuration change. The following configurations are affected:

  - ``type primary`` zones configured with ``dnssec-policy`` but without
    either ``allow-update`` or ``update-policy``,
  - ``type secondary`` zones configured with ``dnssec-policy``.

  In these cases please add ``inline-signing yes;`` to the individual
  zone configuration(s). Without applying this change, :iscman:`named`
  will fail to start. For more details, see
  https://kb.isc.org/docs/dnssec-policy-requires-dynamic-dns-or-inline-signing

- BIND crashes on startup when linked against libuv 1.36. This issue is
  related to ``recvmmsg()`` support in libuv, which was first included
  in libuv 1.35. The problem was addressed in libuv 1.37, but the
  relevant libuv code change requires a special flag to be set during
  library initialization in order for ``recvmmsg()`` support to be
  enabled. This BIND release sets that special flag when required, so
  ``recvmmsg()`` support is now enabled when BIND is compiled against
  either libuv 1.35 or libuv 1.37+; libuv 1.36 is still not usable with
  BIND. :gl:`#1761` :gl:`#1797`

- UDP network ports used for listening can no longer simultaneously be
  used for sending traffic. An example configuration which triggers this
  issue would be one which uses the same address:port pair for
  ``listen-on(-v6)`` statements as for ``notify-source(-v6)`` or
  ``transfer-source(-v6)``. While this issue affects all operating
  systems, it only triggers log messages (e.g. "unable to create
  dispatch for reserved port") on some of them. There are currently no
  plans to make such a combination of settings work again.
