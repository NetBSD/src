.\"	$Id: 1.t,v 1.1 2001/07/04 05:29:25 itojun Exp $
.\"
.\".ds RH 4.4BSD incompatibility with IPv6/IPsec packet processing
.NH 1
4.4BSD incompatibility with IPv6/IPsec packet processing
.PP
The 4.4BSD network code holds a packet in a chain of ``mbuf'' structures.
Each mbuf structure has three flavors:
.IP \(sq
non-cluster header mbuf, which holds MHLEN
(100 bytes in a 32bit architecture installation of 4.4BSD),
.IP \(sq
non-cluster data mbuf, which holds MLEN (104 bytes), and
.IP \(sq
cluster mbuf which holds MCLBYTES (2048 bytes).
.LP
We can make a chain of mbuf structures as a linked list.
Mbuf chains will efficiently hold variable-length packet data.
Such chains also enable us to insert or remove
some of the packet data from the chain
without data copies.
.PP
When processing inbound packets, 4.4BSD uses a function called
.I m_pullup
to ease the manipulation of data content in the mbufs.
It also uses a deep function call tree for inbound packet processing.
While these two items work just fine for traditional IPv4 processing,
they do not work as well with IPv6 and IPsec processing.
.NH 2
Restrictions in 4.4BSD m_pullup
.PP
For input packet processing,
the 4.4BSD network stack uses the
.I m_pullup
function to ease parsing efforts
by adjusting the data content in mbufs for placement onto the continuous memory
region.
.I m_pullup
is defined as follows:
.DS
.SM
\f[CR]struct mbuf *
m_pullup(m, len)
	struct mbuf *m;
	int len;\fP
.DE
.NL
.I m_pullup
will ensure that the first
.I len
bytes in the packet
are placed in the continuous memory region.
After a call to
.I m_pullup,
the caller can safely access the the first
.I len
bytes of the packet, assuming that they are continuous.
The caller can, for example, safely use pointer variables into
the continuous region, as long as they point inside the
.I len
boundary.
.PP
.1C
.KS
.PS
box wid boxwid*1.2 "IPv6 header" "next = routing"
box same "routing header" "next = auth"
box same "auth header" "next = TCP"
box same "TCP header"
box same "TCP payload"
.PE
.ce
.nr figure +1
Figure \n[figure]: IPv6 extension header chain
.KE
.if t .2C
.I m_pullup
makes certain assumptions regarding protocol headers.
.I m_pullup
can only take
.I len
upto MHLEN.
If the total packet header length is longer than MHLEN,
.I m_pullup
will fail, and the result will be a loss of the packet.
Under IPv4,
.[
RFC791
.]
the length assumption worked fine in most cases,
since for almost every protocol, the total length of the protocol header part
was less than MHLEN.
Each packet has only two protocol headers, including the IPv4 header.
For example, the total length of the protocol header part of a TCP packet
(up to TCP data payload) is a maximum of 120 bytes.
Typically, this length is 40 to 48 bytes.
When an IPv4 option is present, it is stripped off before TCP
header processing, and the maximum length passed to
.I m_pullup
will be 100.
.IP 1
The IPv4 header occupies 20 bytes.
.IP 2
The IPv4 option occupies 40 bytes maximum.
It will be stripped off before we parse the TCP header.
Also note that the use of IPv4 options is very rare.
.IP 3
The TCP header length is 20 bytes.
.IP 4
The TCP option is 40 bytes maximum.
In most cases it is 0 to 8 bytes.
.LP
.PP
IPv6 specification
.[
RFC2460
.]
and IPsec specification
.[
RFC2401
.]
allow more flexible use of protocol headers
by introducing chained extension headers.
With chained extension headers, each header has a ``next header field'' in it.
A chain of headers can be made as shown
.nr figure +1
in Figure \n[figure].
.nr figure -1
The type of protocol header is determined by
inspecting the previous protocol header.
There is no restriction in the number of extension headers in the spec.
.PP
Because of extension header chains, there is now no upper limit in
protocol packet header length.
The
.I m_pullup
function would impose unnecessary restriction
to the extension header processing.
In addition,
with the introduction of IPsec, it is now impossible to strip off extension headers
during inbound packet processing.
All of the data on the packet must be retained if it is to be authenticated
using Authentication Header.
.[
RFC2402
.]
Continuing the use of
.I m_pullup
will limit the
number of extension headers allowed on the packet,
and could jeopadize the possible usefulness of IPv6 extension headers. \**
.FS
In IPv4 days, the IPv4 options turned out to be unusable
due to a lack of implementation.
This was because most commercial products simply did not support IPv4 options.
.FE
.PP
Another problem related to
.I m_pullup
is that it tends to copy the protocol header even
when it is unnecessary to do so.
For example, consider the mbuf chain shown
.nr figure +1
in Figure \n[figure]:
.nr figure -1
.KS
.PS
define pointer { box ht boxht*1/4 }
define payload { box }
IP: [
	IPp: pointer
	IPd: payload with .n at bottom of IPp "IPv4"
]
move
TCP: [
	TCPp: pointer
	TCPd: payload with .n at bottom of TCPp "TCP" "TCP payload"
]
arrow from IP.IPp.center to TCP.TCPp.center
.PE
.ce
.nr figure +1
.nr beforepullup \n[figure]
Figure \n[figure]: mbuf chain before \fIm_pullup\fP
.KE
Here, the first mbuf contains an IPv4 header in the continuous region, 
and the second mbuf contains a TCP header in the continuous region.
When we look at the content of the TCP header,
under 4.4BSD the code will look like the following:
.DS
.SM
\f[CR]struct ip *ip;
struct tcphdr *th;
ip = mtod(m, struct ip *);
/* extra copy with m_pullup */
m = m_pullup(m, iphdrlen + tcphdrlen);
/* MUST  reinit ip */
ip = mtod(m, struct ip *);
th = mtod(m, caddr_t) + iphdrlen;\fP
.NL
.DE
As a result, we will get a mbuf chain shown in
.nr figure +1
Figure \n[figure].
.nr figure -1
.KF
.PS
define pointer { box ht boxht*1/4 }
define payload { box }
IP: [
	IPp: pointer
	IPd: payload with .n at bottom of IPp "IPv4" "TCP"
]
move
TCP: [
	TCPp: pointer
	TCPd: payload with .n at bottom of TCPp "TCP payload"
]
arrow from IP.IPp.center to TCP.TCPp.center
.PE
.ce
.nr figure +1
Figure \n[figure]: mbuf chain in figure \n[beforepullup] after \fIm_pullup\fP
.KE
Because
.I m_pullup
is only able to make a continuous
region starting from the top of the mbuf chain,
it copies the TCP portion in second mbuf
into the first mbuf.
The copy could be avoided if
.I m_pullup
were clever enough
to handle this case.
Also, the caller side is required to reinitialize all of
the pointers that point to the content of mbuf,
since after
.I m_pullup,
the first mbuf on the chain
.1C
.KS
.PS
ellipse "\fIip6_input\fP"
arrow
ellipse "\fIrthdr6_input\fP"
arrow
ellipse "\fIah_input\fP"
arrow "stack" "overflow"
ellipse "\fIesp_input\fP"
arrow
ellipse "\fItcp_input\fP"
.PE
.ce
Figure 5: an excessively deep call chain can cause kernel stack overflow
.KE
.if t .2C
.LP
can be reallocated and lives at
a different address than before.
While
.I m_pullup
design has provided simplicity in packet parsing,
it is disadvantageous for protocols like IPv6.
.PP
The problems can be summarized as follows:
(1)
.I m_pullup
imposes too strong restriction 
on the total length of the packet header (MHLEN);
(2)
.I m_pullup
makes an extra copy even when this can be avoided; and
(3)
.I m_pullup
requires the caller to reinitialize all of the pointers into the mbuf chain.
.NH 2
Protocol header processing with a deep function call chain
.PP
Under 4.4BSD, protocol header processing will make a chain of function calls.
For example, if we have an IPv4 TCP packet, the following function call chain will be made
.nr figure +1
(see Figure \n[figure]):
.nr figure -1
.IP (1)
.I ipintr
will be called from the network software interrupt logic,
.IP (2)
.I ipintr
processes the IPv4 header, then calls
.I tcp_input.
.\".I ipintr
.\"can be called
.\".I ip_input
.\"from its functionality.
.IP (3)
.I tcp_input
will process the TCP header and pass the data payload
to the socket queues.
.LP
.KF
.PS
ellipse "\fIipintr\fP"
arrow
ellipse "\fItcp_input\fP"
.PE
.ce
.nr figure +1
Figure \n[figure]: function call chain in IPv4 inbound packet processing
.KE
.PP
If chained extension headers are handled as described above,
the kernel stack can overflow by a deep function call chain, as shown in
.nr figure +1
Figure \n[figure].
.nr figure -1
.nr figure +1
IPv6/IPsec specifications do not define any upper limit
to the number of extension headers on a packet,
so a malicious party can transmit a ``legal'' packet with a large number of chained
headers in order to attack IPv6/IPsec implementations.
We have experienced kernel stack overflow in IPsec code,
tunnelled packet processing code, and in several other cases.
The IPsec processing routines tend to use a large chunk of memory
on the kernel stack, in order to hold intermediate data and the secret keys
used for encryption. \**
.FS
For example, blowfish encryption processing code typically uses
an intermediate data region of 4K or more.
With typical 4.4BSD installation on i386 architecture,
the kernel stack region occupies less than 8K bytes and does not grow on demand.
.FE
We cannot put the intermediate data region into a static data region outside of
the kernel stack,
because it would become a source of performance drawback on multiprocessors
due to data locking.
.PP
Even though the IPv6 specifications do not define any restrictions
on the number of extension headers, it may be possible
to impose additional restriction in an IPv6 implementation for safety.
In any case, it is not possible to estimate the amount of the
kernel stack, which will be used by protocol handlers.
We need a better calling convention for IPv6/IPsec header processing,
regardless of the limits in the number of extension headers we may impose.
