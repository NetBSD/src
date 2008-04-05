$NetBSD: README.NAS,v 1.3 2008/04/05 03:39:41 nisimura Exp $

//// MPC8241/8245 based NAS products ////

The following NAS products are known by NetBSD/sandpoint.

NIC	IDE		machine description
----	----		--------------------------
tlp.11	cmdide.12	classic Kurobox
re.11	cmdide.12	Kurobox HG
re.11	iteide.12/13	classic TeraStation
re.11	satalink.12/13	TeraStation Pro
re.11	iteide.12	Gigabit LinkStation
mak.15	iteide.13	Synology DS-106j/LinkStation LANxxxG
mak.15	satalink.13	Synology DS-101g+/106e/106/207
mak.15	satalink.12/13	Synology CS-406/RS-406/CS-406e/CS-407e
wm.15	satalink.13 	QNAP TS-101/201

EPIC IRQ and PCI line/pin assignments

Kurobox		11, 12, 13, 14	-> 0, 1, 4, 3
Synology	12, 13, 14, 15	-> 4, 0, 1, 2
QNAP		12, 13, 14, 15	-> 0, 1, 2, 3

USB EHCI is a multiple function PCI device which has
pin assignment A, B and C.

[ research still in progress ]

---
