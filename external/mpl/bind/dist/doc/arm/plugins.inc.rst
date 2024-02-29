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

.. _module-info:

Plugins
~~~~~~~

Plugins are a mechanism to extend the functionality of :iscman:`named` using
dynamically loadable libraries. By using plugins, core server
functionality can be kept simple for the majority of users; more complex
code implementing optional features need only be installed by users that
need those features.

The plugin interface is a work in progress, and is expected to evolve as
more plugins are added. Currently, only "query plugins" are supported;
these modify the name server query logic. Other plugin types may be
added in the future.

The only plugin currently included in BIND is :iscman:`filter-aaaa.so <filter-aaaa>`, which
replaces the ``filter-aaaa`` feature that previously existed natively as
part of :iscman:`named`. The code for this feature has been removed from
:iscman:`named` and can no longer be configured using standard :iscman:`named.conf`
syntax, but linking in the :iscman:`filter-aaaa.so <filter-aaaa>` plugin provides identical
functionality.

Configuring Plugins
~~~~~~~~~~~~~~~~~~~
.. namedconf:statement:: plugin
   :tags: server
   :short: Configures plugins in :iscman:`named.conf`.

A plugin is configured with the :any:`plugin` statement in :iscman:`named.conf`:

::

       plugin query "library.so" {
           parameters
       };


In this example, file ``library.so`` is the plugin library. ``query``
indicates that this is a query plugin.

Multiple :any:`plugin` statements can be specified, to load different
plugins or multiple instances of the same plugin.

``parameters`` are passed as an opaque string to the plugin's initialization
routine. Configuration syntax differs depending on the module.

Developing Plugins
~~~~~~~~~~~~~~~~~~

Each plugin implements four functions:

-  ``plugin_register``
   to allocate memory, configure a plugin instance, and attach to hook
   points within
   :iscman:`named`
   ,
-  ``plugin_destroy``
   to tear down the plugin instance and free memory,
-  ``plugin_version``
   to check that the plugin is compatible with the current version of
   the plugin API,
-  ``plugin_check``
   to test syntactic correctness of the plugin parameters.

At various locations within the :iscman:`named` source code, there are "hook
points" at which a plugin may register itself. When a hook point is
reached while :iscman:`named` is running, it is checked to see whether any
plugins have registered themselves there; if so, the associated "hook
action" - a function within the plugin library - is called. Hook
actions may examine the runtime state and make changes: for example,
modifying the answers to be sent back to a client or forcing a query to
be aborted. More details can be found in the file
``lib/ns/include/ns/hooks.h``.
