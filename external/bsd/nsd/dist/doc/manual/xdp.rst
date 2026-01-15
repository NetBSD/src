Using XDP/AF_XDP sockets
========================

..
    TODO: set version
    Since version xxx, NSD has support for AF_XDP sockets.

AF_XDP sockets introduce a fast-path for network packets from the device driver
directly to user-space memory, bypassing the rest of the Linux network stack and
facilitating high packet-rate and bandwidth processing of network packets—DNS
queries (only via UDP) in our case.

Compiling NSD with XDP
----------------------

As this feature is experimental and introduces new dependencies to NSD, it needs
to be compiled in with ``--enable-xdp``.

The additional dependencies are: ``libxdp libbpf libcap clang llvm`` (and
``gcc-multilib`` to fix a missing ``asm/types.h`` file, if you're on Ubuntu).

For Debian/Ubuntu based systems, you would install the dependencies as follows:

.. code-block:: bash

    sudo apt install -y libxdp-dev libbpf-dev libcap-dev clang llvm gcc-multilib

When using the git source repository, make sure to also initialize the
submodules and auxilary files:

.. code-block:: bash

    git clone https://github.com/NLnetLabs/nsd --branch features/af-xdp
    cd nsd
    git submodule update --init
    autoreconf -fi

After installing the dependencies, you can build NSD:

.. code-block:: bash

    ./configure --enable-xdp
    make -j4
    sudo make install


Configuring XDP
---------------

The configuration options are described in `nsd.conf(5) <manpages/nsd.conf.html#xdp>`_.

By default, you can enable XDP for a single interface that supports it, with
the ``xdp-interface`` option:

.. code-block:: text

    server:
        xdp-interface: enp1s0

In this configuration, NSD will load and (after stopping NSD) unload its
bundled XDP program that redirects UDP traffic to port 53 directly to NSD
user-space. You usually don't have to do anything else. If it doesn't work,
check out the :ref:`its-not-working` section.

.. Note::

   Even though NSD uses libxdp, NSD skips the xdp-dispatcher. This is done so
   that NSD can unload the XDP program itself when finished without requiring
   the SYS_ADMIN capability (see `xdp-project/xdp-tools#432
   <https://github.com/xdp-project/xdp-tools/pull/432>`_ and
   `xdp-project/xdp-tools#434
   <https://github.com/xdp-project/xdp-tools/issues/434>`_ on GitHub).
   If you use multiple XDP programs on your system, please refer to
   :ref:`load-bundled-xdp`, until we turn this into a config option.

Configuring XDP in special cases
--------------------------------

If you have custom requirements for the use of XDP, e.g. because you want to
integrate NSD into you existing XDP setup, you have two options:

1. :ref:`load-bundled-xdp`, or
2. :ref:`custom-xdp`.

.. _load-bundled-xdp:

Loading the bundled XDP program yourself
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

NSD includes two XDP programs (in ``/usr/share/nsd/``, by default). You'll need
the file named ``xdp-dns-redirect_kern_pinned.o``. The two programs are
functionally identical, the ``_pinned`` variant just defines the pinning option
for the ``xsks_map``.

When loading the program, make sure to instruct your xdp program loader of
choice to pin the map and adjust the file permissions so that NSD can modify the
pinned map. For example with the ``xdp-loader`` from xdp-tools:

.. code-block:: bash

   sudo xdp-loader load -p /sys/fs/bpf <iface> /usr/share/nsd/xdp-dns-redirect_kern_pinned.o
   sudo chown nsd /sys/fs/bpf/xsks_map
   sudo chmod o+x /sys/fs/bpf

You'll need to instruct NSD to not load any XDP programs, and inform NSD about
which XDP program you loaded and the bpffs path used, if that differs from the
default of ``/sys/fs/bpf``. For example:

.. code-block:: text

    server:
        xdp-interface: enp1s0

        xdp-program-load: no
        xdp-program-path: "/usr/share/nsd/xdp-dns-redirect_kern_pinned.o"
        xdp-bpffs-path: "/sys/fs/bpf"

.. _custom-xdp:

Writing/Extending your own custom XDP program
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If you want to write or extend your own XDP program, you'll need to define
a ``BPF_MAP_TYPE_XSKMAP`` BPF MAP with the name ``xsks_map``:

.. code-block:: c

    struct {
      __uint(type, BPF_MAP_TYPE_XSKMAP);
      __type(key, __u32);
      __type(value, __u32);
      __uint(max_entries, 64); // max_entries must be >= number of network queues
      __uint(pinning, LIBBPF_PIN_BY_NAME);
    } xsks_map SEC(".maps");

Like with :ref:`load-bundled-xdp` (see above), you'll need to pin the map to
a bpffs and configure NSD appropriately.

NSD (the XDP code path) internally checks whether incoming traffic is destined
for port 53. If you want to redirect UDP traffic incoming at a port other than
53, you'll currently have to adjust ``DNS_PORT`` in ``xdp-server.c``
accordingly.

.. _its-not-working:

It's not working
----------------

Some drivers don't support AF_XDP sockets fully. For those you can try out the
``xdp-force-copy`` option:

.. code-block:: text

    server:
        xdp-force-copy: yes
