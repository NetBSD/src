.\"	$Id: 8.t,v 1.1 2001/07/04 05:29:25 itojun Exp $
.\"
.\".ds RH Comparisons
.NH 1
Comparisons
.PP
This section compares the following three approaches in terms of
their characteristics and actual behavior:
(1) 4.4BSD
.I m_pullup,
(2) NRL
.I m_pullup2,
and (3) KAME
.I m_pulldown.
.LP
.NH 2
Comparison of assumption
.PP
Table 1 shows the assumptions made by each of the three approaches.
As mentioned earlier,
.I m_pullup
imposes too stringent requirement for the total length of packet headers.
.I m_pullup2
is workable in most cases, although
this approach adds more restrictions than the specification claims.
.I m_pulldown
assumes that the single packet header is smaller than MCLBYTES,
but makes
no restriction regarding the total length of packet headers.
With a standard mbuf chain,
this is the best
.I m_pulldown
can do, since there is no way to hold continuous region longer than MCLBYTES.
This characteristic can contribute to better specification conformance,
since
.I m_pulldown
will impose fewer additional restrictions due to the
requirements of implementation.
.PP
Among the three approaches, only
.I m_pulldown
avoids making unnecessary copies of intermediate header data and
avoids pointer reinitialization after calls to these functions.
These attributes result in smaller overhead during input packet processing.
.PP
.nr table +1
At present,
we know of no other 4.4BSD-based IPv6/IPsec stack that addresses kernel
stack overflow issues,
although we are open to
new perspectives and new information.
.NH 2
Performance comparison based on simulated statistics
.PP
To compare the behavior and performance of
.I m_pulldown
against
.I m_pullup
and
.I m_pullup2
using the same set of traffic and
mbuf chains, we have gathered simulated statistics for
.I m_pullup
and
.I m_pullup2,
in
.I m_pulldown
function.
By running a kernel using the modified
.I m_pulldown
function,
we can easily
gather statistics for these three functions against exactly the same traffic.
.PP
The comparison was made on a computer
(with Celeron 366MHz CPU, 192M bytes of memory)
running NetBSD 1.4.1 with the KAME IPv6/IPsec stack.
Network drivers allocate mbufs just as normal 4.4BSD does.
.I m_pulldown
is called whenever it is needed to ensure continuity in packet data
during inbound packet processing.
The role of the computer is as an end node, not a router.
.PP
To describe the content of the following table,
we must look at the source code fragment.
.nr figure +1
Figure \n[figure]
.nr figure -1
shows the code fragment from our source code.
The code fragment will
(1) make the TCP header on the mbuf chain
.I m
at offset
.I hdrlen
continuous, and (2) point the region with pointer
.I th.
We use a macro named IP6_EXTHDR_CHECK,
and the code before and after the macro expansion is shown in the figure.
.KF
.LD
.ps 6
.vs 7
\f[CR]/* ensure that *th from hdrlen is continuous */
/* before macro expansion... */
struct tcphdr *th;
IP6_EXTHDR_CHECK(th, struct tcphdr *, m,
	hdrlen, sizeof(*th));
if (th == NULL)
    return;	/*m is already freed*/


/* after macro expansion... */
struct tcphdr *th;
int off;
struct mbuf *n;
if (m->m_len < hdrlen + sizeof(*th)) {
    n = m_pulldown(m, hdrlen, sizeof(*th), &off);
    if (n)
	th = (struct tcphdr *)(mtod(n, caddr_t) + off);
    else
	th = NULL;
} else
    th = (struct tcphdr *)(mtod(m, caddr_t) + hdrlen);
if (th == NULL)
    return;\fP
.NL
.DE
.nr figure +1
Figure \n[figure]: code fragment for trimming mbuf chain.
.KE
In Table 2,
the first column identifies the test case.
The second column shows the number of times
the IP6_EXTHDR_CHECK macro was used.
In other words, it shows the number of times we have made checks against
mbuf length.
The remaining columns show, from left to right,
the number of times memory allocation/copy was performed in each of the variants.
In the case of
.I m_pullup,
we counted the number of cases we passed
.I len
in excess of MHLEN (96 bytes in this installation).
.\"With
.\".I m_pullup2
.\"and
.\".I m_pulldown,
.\"there were no such failures.
This result suggests
that there was no packet with a packet header portion larger than
MCLBYTES (2048 bytes).
.\" The percentage in parentheses is ratio against the number on the first column.
In the evaluation we have used
.I m_pulldown
against IPv6 traffic only.
.1C
.KF
.TS
center box;
l cfI cfI cfI
l c c c.
	m_pullup	m_pullup2	m_pulldown
_
total header length	MHLEN(100)	MCLBYTES(2048)	\(mi
single header length	\(mi	\(mi	MCLBYTES(2048)
_
T{
avoids copy on intermediate headers
T}	no	no	yes
_
T{
avoids pointer reinitialization
T}	no	no	yes
.TE
.ce
Table 1: assumptions in mbuf manipulation approaches.
.KE
.KF
.TS
center box;
c |c |cfI s s |cfI s s |cfI s
c |r |c c c |c c c |c c
r |r |r r r |r r r |r r.
test	len checks	m_pulldown	m_pullup	m_pullup2
		call	alloc	copy	alloc	copy	fail	alloc	copy
_
(1)	204923	1706	1595	1596	165	165	1541	1596	1596
(2)	1063995	23786	22931	23008	1171	1229	22557	22895	22953
(3)	520028	1245	948	957	432	432	813	945	945
(4)	438602	180	6	6	178	178	2	24	24
(5)	5570	2236	206	206	812	812	1424	1424	1424
.TE
.ce
Table 2: number of mbuf allocation/copy against traffic
.KE
.KF
.TS
center box;
c |c c c c |c c c
c |r r r r |r r r.
test	IPv6 input	TCP	UDP	ICMPv6	1 mbuf	2 mbufs	ext mbuf(s)
_
(1)	29334	20892	2699	5739	3624	15632	10078
(2)	313218	215919	15930	80263	38751	172976	101491
(3)	132267	117822	8561	5882	12782	59799	59686
(4)	73160	66512	5249	1343	7475	42053	23632
(5)	1433	148	53	52	103	1203	127
.TE
.ce
Table 3: Traffic characteristics for tests in Table 2
.KE
.if t .2C
.PP
From these measured results, we obtain several interesting observations.
.I m_pullup
actually failed on IPv6 trafic.
If an IPv6 implementation uses
.I m_pullup
for IPv6 input processing,
it must be coded carefully so as to avoid trying
.I m_pullup
against any length longer than MHLEN.
To achieve this end, the code copies the data portion from the mbuf
chain to a separate buffer, and the cost of memory copies becomes a penalty.
.PP
Due to the nature of this simulation,
the comparison described above may contain an implicit bias.
Since the IPv6 protocol processing code is written by using
.I m_pulldown,
the code is somewhat biased toward
.I m_pulldown.
If a programmer had to write the entire IPv6 protocol processing with
.I m_pullup
only, he or she would use
.I m_copydata
to copy intermediate
extension headers buried deep inside the header chains,
thus making it unnecessary to call
.I m_pullup.
In any case, a call to
.I m_copydata
will result in a data copy,
which causes extra overhead.
.\"The author thinks that this bias toward
.\".I m_pulldown
.\"is therefore negligible.
.PP
In all cases, the number of length checks (second column) exceeds the
number of inbound packets.
This behavior is the same as in the original 4.4BSD stack;
we did not add a significant number of length checks to the code.
This is because
.I m_pulldown
(or
.I m_pullup
in the 4.4BSD case)
is called
as necessary during the parsing of the headers.
For example, to process a TCP-over-IPv6 packet, at least 3
checks would be made against m->m_len;
these checks would be made
to grab the IPv6 header (40 bytes),
to grab the TCP header (20 bytes), and to grab the TCP header
and options (20 to 60 bytes).
The length of the TCP option part is kept inside the TCP header,
so the length needs to be checked twice for the TCP part.
.\"If the function call overhead is more significant than the actual
.\".I m_pullup
.\"or
.\".I m_pulldown
.\"operation,
.\"we may be able to blindly call
.\".I m_pulldown
.\"with the maximum TCP option length
.\"(60 bytes) in order to reduce the number of function calls.
.KF
.PS
Ao:	box invis ht boxht*2
A:	box at center of Ao "IPv6 header"
Bo:	box invis ht boxht*2
B:	box at center of Bo "TCP header" "(len)"
Co:	box invis ht boxht*2
C:	box at center of Co "TCP options"
D:	box "payload"

arrow from 1/3 of the way between Ao.sw and Ao.se to Ao.sw
arrow from 2/3 of the way between Ao.sw and Ao.se to Ao.se
line invis from Ao.sw to Ao.se "40"
line from Ao.sw to 4/5 of the way between Ao.sw and A.sw
line from Ao.se to 4/5 of the way between Ao.se and A.se

arrow from 1/3 of the way between Bo.nw and Bo.ne to Bo.nw
arrow from 2/3 of the way between Bo.nw and Bo.ne to Bo.ne
line invis from Bo.nw to Bo.ne "20"
line from Bo.nw to 4/5 of the way between Bo.nw and B.nw
line from Bo.ne to 4/5 of the way between Bo.ne and B.ne

arrow from 1/3 of the way between Bo.sw and Co.se to Bo.sw
arrow from 2/3 of the way between Bo.sw and Co.se to Co.se
line invis from Bo.sw to Co.se "20 to 60"
line from Bo.sw to 4/5 of the way between Bo.sw and B.sw
line from Co.se to 4/5 of the way between Co.se and C.se
.PE
.ce
.nr figure +1
Figure \n[figure]: processing a TCP-over-IPv6 packet requires 3 length checks.
.KE
The results suggest that we call
.I m_pulldown
more frequently in ICMPv6 processing than in the processing of other protocols.
These additional calls are made for parsing of ICMPv6 and for neighbor discovery options.
The use of loopback interface also contributes to the use of
.I m_pulldown.
.PP
In the tests, the number of copies made in the
.I m_pullup2
case is similar to the number made in the
.I m_pulldown
case.
.I m_pulldown
makes less copies than
.I m_pullup2
against packets like below:
.IP \(sq
A packet is kept in multiple mbuf.
With mbuf allocation policy in
.I m_devget,
we will see two mbufs to hold single packet
if the packet is larger than MHLEN and smaller than MHLEN + MLEN,
or the packet is larger than MCLBYTES.
.IP \(sq
We have extension headers in multiple mbufs.
Header portion in the packet needs to occupy first mbuf and
subsequent mbufs.
.LP
To demonstrate the difference, we have generated an IPv6 packet with a
routing header, with 4 IPv6 addresses.
The test result is presented as the 5th test in Table 2.
Packet will look like
.nr figure +1
Figure \n[figure].
.nr figure -1
First 112 bytes are occupied by an IPv6 header and a routing header,
and the remaining 16 bytes are used for an ICMPv6 header and payload.
The packet met the above condition, and
.I m_pulldown
made less copies than
.I m_pullup2.
To process single incoming ICMPv6 packet shown in the figure,
.I m_pullup2
made 7 copies while
.I m_pulldown
made only 1 copy.
.KF
.LD
.ps 6
.vs 7
\f[CR]node A (source) = 2001:240:0:200:260:97ff:fe07:69ea
node B (destination) = 2001:240:0:200:a00:5aff:fe38:6f86
17:39:43.346078 A > B:
	srcrt (type=0,segleft=4,[0]B,[1]B,[2]B,[3]B):
	icmp6: echo request (len 88, hlim 64)
		 6000 0000 0058 2b40 2001 0240 0000 0200
		 0260 97ff fe07 69ea 2001 0240 0000 0200
		 0a00 5aff fe38 6f86 3a08 0004 0000 0000
		 2001 0240 0000 0200 0a00 5aff fe38 6f86
		 2001 0240 0000 0200 0a00 5aff fe38 6f86
		 2001 0240 0000 0200 0a00 5aff fe38 6f86
		 2001 0240 0000 0200 0a00 5aff fe38 6f86
		 8000 b650 030e 00c8 ce6e fd38 d553 0700
.DE
.ce
.nr figure +1
Figure \n[figure]: Packets with IPv6 routing header.
.KE
.PP
During the test, we experienced no kernel stack overflow,
thanks to a new calling sequence between IPv6 protocol handlers.
.PP
The number of copies and mbuf allocations vary very much by tests.
We need to investigate the traffic characteristic more carefully,
for example, about the average length of header portion in packets.
