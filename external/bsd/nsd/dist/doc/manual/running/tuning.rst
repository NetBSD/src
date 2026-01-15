Tuning
======

In version 4.3.0 of NSD, additional functionality was added to increase
performance even more. Most notably, this includes processor affinity.

NSD is performant by design because it matters when operators serve hundreds of
thousands or even millions of queries per second. We strive to make the right
choices by default, like enabling the use of ``libevent`` at the configure stage
to ensure the most efficient event mechanism is used on a given platform. e.g.
``epoll`` on Linux and ``kqueue`` on FreeBSD. Switches are available for
operators who know the implementation on their system behaves correctly, like
enabling the use of ``recvmmsg`` at the configure stage
(`--enable-recvmmsg`) to read multiple messages from a socket in one system
call.

By default NSD forks (only) one server. Modern computer systems however, may
have more than one processor, and usually have more than one core per processor.
The easiest way to scale up performance is to simply fork more servers by
configuring server-count: to match the number of cores available in the system
so that more queries can be answered simultaneously. If the operating system
supports it, ensure ``reuseport:`` is set to ``yes`` to distribute incoming
packets evenly across server processes to balance the load.

A couple of other options that the operator may want to consider:

1. TCP capacity can be significantly increased by setting ``tcp-count: 1000``
   and ``tcp-timeout: 3``. Set ``tcp-reject-overflow: yes`` to prevent the
   kernel connection queue from growing.

Processor Affinity
------------------

The aforementioned settings provide an easy way to increase performance without
the need for in-depth knowledge of the hardware. For operators that require even
more throughput ``cpu-affinity`` is available.

The operating system’s scheduling-algorithm determines which core a given task
is allocated to. Processors build up state — e.g. by keeping frequently accessed
data in cache memory — for the task that it is currently executing. Whenever a
task switches cores, performance is degraded because the core it switched to has
yet to build up said state. While this scheduling-algorithm works just fine for
general-purpose computing, operators may want to designate a set of cores for
best performance. The ``cpu-affinity`` family of configuration options was added
to NSD specifically for that purpose.

Processor affinity is currently supported on Linux and FreeBSD. Other operating
systems may be supported in the future, but not all operating systems that can
run NSD support CPU pinning. To fully benefit from this feature, one must first
determine which cores should be allocated to NSD. This requires some knowledge
of the underlying hardware, but generally speaking every process should run on a
dedicated core and the use of Hyper-Threading cores should be avoided to prevent
resource contention. List every core designated to NSD in ``cpu-affinity`` and
bind each server process to a specific core using ``server-<N>-cpu-affinity``
and ``xfrd-cpu-affinity`` to improve L1/L2 cache hit rates and reduce pipeline
stalls/flushes.

.. code:: text

    server:
      server-count: 2
      cpu-affinity: 0 1 2
      server-1-cpu-affinity: 0
      server-2-cpu-affinity: 1
      xfrd-cpu-affinity: 2

Partition Sockets
-----------------

``ip-address:`` options in the ``server:`` clause can be configured per server
or set of servers. Sockets configured for a specific server are closed by other
servers on startup. This improves performance if a large number of sockets are
scanned using ``select/poll`` and avoids waking up multiple servers when a
packet comes in, known as the `thundering herd problem
<https://en.wikipedia.org/wiki/Thundering_herd_problem>`_. Though both problems
are solved using a modern kernel and a modern I/O event mechanism, there is one
other reason to partition sockets, explained below.

.. code:: text

    server:
      ip-address: 192.0.2.1 servers=1

Bind to Device
--------------

``ip-address:`` options in the server: clause can now also be configured to bind
directly to the network interface device on Linux (``bindtodevice=yes``) and to
use a specific routing table on FreeBSD (``setfib=<N>``). These were added to
ensure UDP responses go out over the same interface the query came in on if
there are multiple interfaces configured on the same subnet, but there may be
some performance benefits as well as the kernel does not have to go through the
network interface selection process.

.. code:: text

    server:
      ip-address: 192.0.2.1 bindtodevice=yes setfib=<N>

.. Note:: FreeBSD does not create extra routing tables on demand. Consult the
          FreeBSD Handbook, forums, etc. for information on how to configure
          multiple routing tables.

Combining Options
-----------------

Field tests have shown best performance is achieved by combining the
aforementioned options so that each network interface is essentially bound to a
specific core. To do so, use one IP address per server process, pin that process
to a designated core and bind directly to the network interface device.

.. code:: text

    server:
      server-count: 2
      cpu-affinity: 0 1 2
      server-1-cpu-affinity: 0
      server-2-cpu-affinity: 1
      xfrd-cpu-affinity: 2
      ip-address: 192.0.2.1 servers=1 bindtodevice=yes setfib=1
      ip-address: 192.0.2.2 servers=2 bindtodevice=yes setfib=2

The above snippet serves as an example on how to use the configuration options.
Which cores, IP addresses and routing tables are best used depends entirely on
the hardware and network layout. Be sure to test extensively before using the
options.
