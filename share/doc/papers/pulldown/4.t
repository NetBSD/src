.\"	$Id: 4.t,v 1.1 2001/07/04 05:29:25 itojun Exp $
.\"
.\".ds RH Alternative approaches
.NH 1
Alternative approaches
.PP
Many BSD-based IPv6 stacks have been implemented.
While the most popular stacks include NRL, INRIA and KAME,
dozens of other BSD-based IPv6 implementations have been made.
This section presents alternative approaches for purposes of comparison.
.NH 2
NRL m_pullup2
.PP
The latest NRL IPv6 release copes with the
.I m_pullup
limitation by introducing a new function,
.I m_pullup2.
.I m_pullup2
works similarly to
.I m_pullup,
but it allows
.I len
to extend up to MCLBYTES, which corresponds to 2048 bytes in a typical installation.
When
the
.I len
parameter is smaller than or equal to MHLEN,
.I m_pullup2
simply calls
.I m_pullup
from the inside.
.PP
While
.I m_pullup2
works well for packet headers up to MCLBYTES with very little change
in code, it does not avoid making unnecessary copies.
It also imposes restrictions on the total length of packet headers.
The assumption here is that the total length of packet headers is less than
MCLBYTES.
.NH 2
Hydrangea changes to m_devget
.PP
The Hydrangea IPv6 stack was implemented by a group of Japanese researchers,
and is one of the ancestors of the KAME IPv6 stack.
The Hydrangea IPv6 stack avoids the need for
.I m_pullup
by modifying the mbuf allocation policy in drivers.
For inbound packets, the drivers allocate mbufs by using the
.I m_devget
function, or by re-implementing the behavior of
.I m_devget.
.I m_devget
allocates mbuf as follows:
.IP 1
If the packet fits in MHLEN (100 bytes), allocate a single non-cluster mbuf.
.IP 2
If the packet is larger than MHLEN but fits in MHLEN + MLEN (204 bytes),
allocate two non-cluster mbufs.
.IP 3
Otherwise, allocate multiple cluster mbufs, MCLBYTES (2048 bytes) in size.
.LP
For typical packets, the second case is where
.I m_pullup
is used.
The Hydrangea stack avoids the use of
.I m_pullup
by eliminating the second case.
.PP
This approach worked well in most cases, but failed for (1) loopback interface,
(2) tunnelled packets, and (3) non-conforming drivers.
With the Hydrangea approach, every device driver had to be examined
to ensure the new mbuf allocation policy.
We could not be sure if the constraint was guaranteed until we checked the
driver code,
and the Hydrangea approach raised many support issues.
This was one of our motivations for introducing
.I m_pulldown.
