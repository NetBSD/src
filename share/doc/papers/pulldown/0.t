.\"	$Id: 0.t,v 1.1 2001/07/04 05:29:25 itojun Exp $
.\"
.EQ
delim $$
.EN
.if n .ND
.TL
Mbuf issues in 4.4BSD IPv6/IPsec support
.br
\(em experiences from KAME IPv6/IPsec implemntation \(em
.AU
Jun-ichiro itojun Hagino
.AI
KAME Project
Research Laboratory, Internet Initiative Japan Inc.
\f[CR]http://www.kame.net/\fP
.I itojun@iijlab.net
.AB
The 4.4BSD network stack has made certain assumptions regarding the packets it will handle.
In particular, 4.4BSD assumes that
(1) the total protocol header length is shorter than or equal to MHLEN,
usually 100 bytes, and
(2) there are a limited number of protocol headers on a packet.
Neither of these assumptions hold any longer,
due to the way IPv6/IPsec specifications are written.
.PP
We at the KAME project
are implementing IPv6 and IPsec support code on top of 4.4BSD.
To cope with the problems, we have introduced the following changes:
(1) a new function called
.I m_pulldown,
which adjusts the mbuf chain with a minimal number of copies/allocations, and
(2) a new calling sequence for parsing inbound packet headers.
These changes allow us to manipulate incoming packets in a safer,
more efficient, and more spec-conformant way.
The technique described in this paper is integrated into the KAME IPv6/IPsec
stack kit, and is freely available under BSD copyright.
The KAME codebase is being merged into NetBSD, OpenBSD and FreeBSD.
An integration into BSD/OS is planned.
.AE
.\".LP
.de PT
.lt \\n(LLu
.pc %
.nr PN \\n%
.tl '\\*(LH'\\*(CH'\\*(RH'
.lt \\n(.lu
..
.\".af PN i
.\".ce
.\".B "TABLE OF CONTENTS"
.\".LP
.\".sp 1
.\".nf
.\".B "1.  Introduction"
.\".LP
.\".sp .5v
.\".nf
.\".B "2.  The \fIgprof\fP Profiler"
.\"\0.1.    Data Presentation"
.\"\0.1.1.   The Flat Profile
.\"\0.1.2.   The Call Graph Profile
.\"\0.2     Profiling the Kernel
.\".LP
.\".sp .5v
.\".nf
.\".B "3.  Using \fIgprof\fP to Improve Performance
.\"\0.1.    Using the Profiler
.\"\0.2.    An Example of Tuning
.\".LP
.\".sp .5v
.\".nf
.\".B "4.  Conclusions"
.\".LP
.\".sp .5v
.\".nf
.\".B Acknowledgements
.\".LP
.\".sp .5v
.\".nf
.\".B References
.\".af PN 1
.ds CH
.ds LH
.ds RH
.\".ds LH mbuf issues in 4.4BSD IPv6 support
.\".ds RH Contents
.\".bp 1
.ds CF
.ds LF
.ds RF
.\".if t .ds CF Freenix2000
.\".if t .ds LF
.\".if t .ds RF Jun-ichiro itojun Hagino
.\".bp 1
.de _d
.if t .ta .6i 2.1i 2.6i
.\" 2.94 went to 2.6, 3.64 to 3.30
.if n .ta .84i 2.6i 3.30i
..
.de _f
.if t .ta .5i 1.25i 2.5i
.\" 3.5i went to 3.8i
.if n .ta .7i 1.75i 3.8i
..
.nr figure 0
.nr table 0
.if t .2C
