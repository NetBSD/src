.\"	$Id: 2.t,v 1.1 2001/07/04 05:29:25 itojun Exp $
.\"
.\".ds RH KAME approach
.NH 1
KAME approach
.PP
This section describes the approaches we at the KAME project
took against the problems mentioned in the previous section.
We introduce a new function called
.I m_pulldown,
in place of
.I m_pullup,
for adjusting payload data in the mbuf.
We also change the calling sequence for the protocol input function.
.NH 2
What is the KAME project?
.PP
In the early days of IPv6/IPsec development,
the Japanese research community felt it very important to make
a reference code available in a freely-redistributable form
for educational, research and deployment purposes.
The KAME project is a consortium of 7 Japanese companies and
an academic research group.
The project aims to deliver IPv6/IPsec reference implementation
for 4.4BSD, under BSD license.
The KAME project intends to deliver the most
spec-conformant IPv6/IPsec implementation possible.
.NH 2
m_pulldown function
.PP
Here we introduce a new function,
.I m_pulldown,
to address the 3 problems with
.I m_pullup
that we have described above.
The actual source code is included at the end of this paper.
The function prototype is as follows:
.DS
.SM
\f[CR]struct mbuf *
m_pulldown(m, off, len, offp)
	struct mbuf *m;
	int off, len;
	int *offp;\fP
.NL
.DE
.I m_pulldown
will ensure that the data region in the mbuf chain,
starting at
.I off
and ending at
.I "off + len",
is put into a continuous memory region.
.I len
must be smaller than, or equal to, MCLBYTES (2048 bytes).
The function returns a pointer to an intermediate mbuf in the chain
(we refer to the pointer as \fIn\fP), and puts the new offset in
.I n
to
.I *offp.
If
.I offp
is NULL, the resulting region can be located by
.I "mtod(n, caddr_t)";
if
.I offp
is non-null, it will be located at
.I "mtod(n, caddr_t) + *offp".
The mbuf prior to
.I off
will remain untouched,
so it is safe to keep the pointers to the mbuf chain.
For example, consider the mbuf chain
.nr figure +1
on Figure \n[figure]
.nr figure -1
as the input.
.KF
.PS
define pointer { box ht boxht*1/4 }
define payload { box }
IP: [
	IPp: pointer
	IPd: payload with .n at bottom of IPp "mbuf1" "50 bytes"
]
move
TCP: [
	TCPp: pointer
	TCPd: payload with .n at bottom of TCPp "mbuf2" "20 bytes"
]
arrow from IP.IPp.center to TCP.TCPp.center
.PE
.ce
.nr figure +1
Figure \n[figure]: mbuf chain before the call to \fIm_pulldown\fP
.KE
If we call
.I m_pulldown
with
.I "off = 40",
.I "len = 10",
and a non-null
.I offp,
the mbuf chain will remain unchanged.
The return value will be a pointer to mbuf1, and
.I *offp
will be
filled with 40.
If we call
.I m_pulldown
with
.I "off = 40",
.I "len = 20",
and null
.I offp,
then the mbuf chain will be modified as shown
.nr figure +1
in Figure \n[figure],
.nr figure -1
by allocating a new mbuf, mbuf3,
into the middle and moving data from both mbuf1 and mbuf2.
The function returns a pointer to mbuf3.
.KF
.PS
define pointer { box ht boxht*1/4 }
define payload { box }
IP: [
	IPp: pointer
	IPd: payload with .n at bottom of IPp "mbuf1" "40 bytes"
]
move 0.2;
INT: [
	INTp: pointer
	INTd: payload with .n at bottom of INTp "mbuf3" "20 bytes"
]
move 0.2;
TCP: [
	TCPp: pointer
	TCPd: payload with .n at bottom of TCPp "mbuf2'" "10 bytes"
]
arrow from IP.IPp.center to INT.INTp.center
arrow from INT.INTp.center to TCP.TCPp.center
.PE
.ce
.nr figure +1
Figure \n[figure]: mbuf chain after call to \fIm_pulldown\fP, with \fIoff = 40\fP and \fIlen = 20\fP
.KE
The
.I m_pulldown
function solves all 3 problems in
.I m_pullup
that were described in the previous section.
.I m_pulldown
does not copy mbufs when copying is not necessary.
Since it does not modify the mbuf chain prior to the speficied offset
.I off,
it is not necessary for the caller to re-initialize the pointers into the mbuf data
region.
With
.I m_pullup,
we always needed to specify the data payload length, starting from the very first byte
in the packet.
With
.I m_pulldown,
we pass
.I off
as the offset to the data payload we are interested in.
This change avoids extra data manipulation when we are only interested in
the intermediate data portion of the packet.
It also eases the assumption regarding total packet header length.
While
.I m_pullup
assumes that the total packet header length is smaller than or equal to MHLEN
(100 bytes),
.I m_pulldown
assumes that single packet header length is smaller than or equal to MCLBYTES
(2048 bytes).
With mbuf framework this is the best we
can do, since there is no way to hold continuous region longer than
MCLBYTES in a standard mbuf chain.
.NH 2
New function prototype for inbound packet processing
.PP
For IPv6 processing, our code does not make a deep function call chain.
Rather, we make a loop in the very last part of
.I ip6_input,
as shown in Figure 8.
IPPROTO_DONE is a pseudo-protocol type value that identifies the end of the
extension header chain.
If more protocol headers exist,
each header processing code will update the pointer variables
and return the next extension header type.
If the final header in the chain has been reached,
IPPROTO_DONE is returned.
.\" figure 8
.nr figure +1
With this code, we no longer have a deep call chain for IPv6/IPsec processing.
Rather,
.I ip6_input
will make calls to each extension header processor
directly.
This avoids the possibility of overflowing the kernel stack due to multiple
extension header processing.
.KF
.PS
A: ellipse "\fIip6_input\fP"
right
move
move
up
move
B: ellipse "\fIrthdr6_input\fP"
move to last ellipse .s
down
C: ellipse "\fIah_input\fP"
D: ellipse "\fIesp_input\fP"
E: ellipse "\fItcp_input\fP"

arrow from 1/4 <A.e, A.ne> to 1/4 <B.w, B.nw>
arrow from 1/4 <B.w, B.sw> to 1/4 <A.e, A.se>

arrow from 1/4 <A.e, A.ne> to 1/4 <C.w, C.nw>
arrow from 1/4 <C.w, C.sw> to 1/4 <A.e, A.se>

arrow from 1/4 <A.e, A.ne> to 1/4 <D.w, D.nw>
arrow from 1/4 <D.w, D.sw> to 1/4 <A.e, A.se>

arrow from 3/8 <A.e, A.ne> to 1/4 <E.w, E.nw>
arrow from 3/8 <E.w, E.sw> to 1/4 <A.e, A.se>
.PE
.ce
.nr figure +1
Figure \n[figure]: KAME avoids function call chain by making a loop in \fIip6_input\fP
.KE
.PP
Regardless of the calling sequence imposed by the
.I pr_input
function prototype, it is important not to use up the kernel
stack region in protocol handlers.
Sometimes it is necessary to decrease the size of kernel stack usage
by using pointer variables and dynamically allocated regions.
.1C
.KF
.DS
.ps 8
.vs 9
\f[CR]struct ip6protosw {
	int (*pr_input) __P((struct mbuf **, int *, int));
	/* and other members */
};

ip6_input(m)
	struct mbuf *m;
{
	/* in the very last part */
	extern struct ip6protosw inet6sw[];
	/* the first one in extension header chain */
	nxt = ip6.ip6_nxt;
	while (nxt != IPPROTO_DONE)
		nxt = (*inet6sw[ip6_protox[nxt]].pr_input)(&m, &off, nxt);
}

/* in each header processing code */
int
foohdr_input(mp, offp, proto)
	struct mbuf **mp;
	int *offp;
	int proto;
{
	/* some processing, may modify mbuf chain */

	if (we have more header to go) {
		*mp = newm;
		*offp = nxtoff;
		return nxt;
	} else {
		m_freem(newm);
		return IPPROTO_DONE;
	}
}\fP
.DE
.NL
.ce
Figure 8: KAME IPv6 header chain processing code.
.KE
.if t .2C
