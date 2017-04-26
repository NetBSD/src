# Building dhcpcd

This attempts to document various ways of building dhcpcd for your
platform.

Building for distribution (ie making a dhcpcd source tarball) now requires
gmake-4 or any BSD make.

## Size is an issue
To compile small dhcpcd, maybe to be used for installation media where
size is a concern, you can use the `--small` configure option to enable
a reduced feature set within dhcpcd.
Currently this just removes non important options out of
`dhcpcd-definitions.conf`, the logfile option and
support for DHCPv6 Prefix Delegation.
Other features maybe dropped as and when required.
dhcpcd can also be made smaller by removing the IPv4 or IPv6 stack:
  *  `--disable-inet`
  *  `--disable-inet6`

Or by removing the following features:
  *  `--disable-auth`
  *  `--disable-arp`
  *  `--disable-arping`
  *  `--disable-ipv4ll`
  *  `--disable-dhcp6`

You can also move the embedded extended configuration from the dhcpcd binary
to an external file (LIBEXECDIR/dhcpcd-definitions.conf)
  *  `--disable-embedded`
If dhcpcd cannot load this file at runtime, dhcpcd will work but will not be
able to decode any DHCP/DHCPv6 options that are not defined by the user
in /etc/dhcpcd.conf. This does not really change the total on disk size.

## Cross compiling
If you're cross compiling you may need set the platform if OS is different
from the host.  
`--target=sparc-sun-netbsd5.0`

If you're building for an MMU-less system where fork() does not work, you
should `./configure --disable-fork`.
This also puts the `--no-background` flag on and stops the `--background` flag
from working.

## Default directories
You can change the default dirs with these knobs.
For example, to satisfy FHS compliance you would do this:
`./configure --libexecdir=/lib/dhcpcd dbdir=/var/lib/dhcpcd`

## Compile Issues
We now default to using `-std=c99`. For 64-bit linux, this always works, but
for 32-bit linux it requires either gnu99 or a patch to `asm/types.h`.
Most distros patch linux headers so this should work fine.
linux-2.6.24 finally ships with a working 32-bit header.
If your linux headers are older, or your distro hasn't patched them you can
set `CSTD=gnu99` to work around this.

ArchLinux presently sanitises all kernel headers to the latest version
regardless of the version for your CPU. As such, Arch presently ships a
3.12 kernel with 3.17 headers which claim that it supports temporary address
management and no automatic prefix route generation, both of which are
obviously false. You will have to patch support either in the kernel or
out of the headers (or dhcpcd itself) to have correct operation.

## OS specific issues
Some BSD systems do not allow the manipulation of automatically added subnet
routes. You can find discussion here:
    http://mail-index.netbsd.org/tech-net/2008/12/03/msg000896.html
BSD systems where this has been fixed or is known to work are:
    NetBSD-5.0
    FreeBSD-10.0

Some BSD systems protect against IPv6 NS/NA messages by ensuring that the
source address matches a prefix on the recieved by a RA message.
This is an error as the correct check is for on-link prefixes as the
kernel may not be handling RA itself.
BSD systems where this has been fixed or is known to work are:
    NetBSD-7.0
    OpenBSD-5.0
    patch submitted against FreeBSD-10.0

Some BSD systems do not announce IPv6 address flag changes, such as
`IN6_IFF_TENTATIVE`, `IN6_IFF_DUPLICATED`, etc. On these systems,
dhcpcd will poll a freshly added address until either `IN6_IFF_TENTATIVE` is
cleared or `IN6_IFF_DUPLICATED` is set and take action accordingly.
BSD systems where this has been fixed or is known to work are:
    NetBSD-7.0

OpenBSD will always add it's own link-local address if no link-local address
exists, because it doesn't check if the address we are adding is a link-local
address or not.

Some BSD systems do not announce cached neighbour route changes based
on reachability to userland. For such systems, IPv6 routers will always
be assumed to be reachable until they either stop being a router or expire.
BSD systems where this has been fixed or is known to work are:
    NetBSD-7.99.3

Linux prior to 3.17 won't allow userland to manage IPv6 temporary addresses.
Either upgrade or don't allow dhcpcd to manage the RA,
so don't set either `ipv6ra_own` or `slaac private` in `dhcpcd.conf` if you
want to have working IPv6 temporary addresses.
SLAAC private addresses are just as private, just stable.

## Init systems
We try and detect how dhcpcd should interact with system services at runtime.
If we cannot auto-detect how do to this, or it is wrong then
you can change this by passing shell commands to `--serviceexists`,
`--servicecmd` and optionally `--servicestatus` to `./configure` or overriding
the service variables in a hook.


## /dev management
Some systems have `/dev` management systems and some of these like to rename
interfaces. As this system would listen in the same way as dhcpcd to new
interface arrivals, dhcpcd needs to listen to the `/dev` management sytem
instead of the kernel. However, if the `/dev` management system breaks, stops
working, or changes to a new one, dhcpcd should still try and continue to work.
To facilitate this, dhcpcd allows a plugin to load to instruct dhcpcd when it
can use an interface. As of the time of writing only udev support is included.
You can disable this with `--without-dev`, or `without-udev`.
NOTE: in Gentoo at least, `sys-fs/udev` as provided by systemd leaks memory
`sys-fs/eudev`, the fork of udev does not and as such is recommended.

## select
dhcpcd uses eloop.c, which is a portable main event loop with timeouts and
signal handling. Unlike libevent and similar, it can be transplanted directly
within the application - the only caveat outside of POSIX calls is that
you provide queue.h based on a recent BSD (glibc sys/queue.h is not enough).
eloop supports the following polling mechanisms, listed in order of preference:
	kqueue, epoll, pollts, ppoll and pselect.
If signal handling is disabled (ie in RTEMS or other single process
OS's) then eloop can use poll.
You can decide which polling mechanism dhcpcd will select in eloop like so
`./configure --with-poll=[kqueue|epoll|pselect|pollts|ppoll]`


## Importing into another source control system
To prepare dhcpcd for import into a platform source tree (like NetBSD)
you can use the make import target to create /tmp/dhcpcd-$version and
populate it with all the source files and hooks needed.
In this instance, you may wish to disable some configured tests when
the binary has to run on older versions which lack support, such as getline.
`./configure --without-getline`


## Hooks
Not all the hooks in dhcpcd-hooks are installed by default.
By default we install `01-test`, `02-dump`, `10-mtu`, `20-resolv.conf`
and `30-hostname`.
The other hooks, `10-wpa_supplicant`, `15-timezone` and `29-lookup-hostname`
are installed to `$(datadir)/dhcpcd/hooks` by default and need to be
copied to `$(libexecdir)/dhcpcd-hooks` for use.
The configure program attempts to find hooks for systems you have installed.
To add more simply
`./configure -with-hook=ntp.conf`

Some system services expose the name of the service we are in,
by default dhcpcd will pick `RC_SVCNAME` from the environment.
You can override this in `CPPFLAGS+= -DRC_SVCNAME="YOUR_SVCNAME"`.
This is important because dhcpcd will scrub the environment aside from `$PATH`
before running hooks.
This variable could be used to facilitate service re-entry so this chain could
happen in a custom OS hook:
  dhcpcd service marked inactive && dhcpcd service starts
  dependant services are not started because dhcpcd is inactive (not stopped)
  dhcpcd hook tests if `$if_up = true` and `$af_waiting` is empty or unset.
  if true, mark the dhcpcd service as started and then start dependencies
  if false and the dhcpcd service was previously started, mark as inactive and
     stop any dependant services.

