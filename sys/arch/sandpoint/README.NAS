$NetBSD: README.NAS,v 1.5 2010/05/08 18:48:44 phx Exp $

//// MPC8241/8245 NAS products ////

The following NAS products are known by NetBSD/sandpoint.

NIC	IDE		machine description
----	----		--------------------------
tlp.11	cmdide.12	classic Kurobox
re.11	cmdide.12	Kurobox HG
re.11	iteide.12/13	classic TeraStation
re.11	satalink.12/13	TeraStation Pro
re.11	iteide.12	Gigabit LinkStation
sk.15	iteide.13	Synology DS-106j/LinkStation LANxxxG
sk.15	satalink.13	Synology DS-101g+/106e/106/207
sk.15	satalink.12/13	Synology CS-406/RS-406/CS-406e/CS-407e
wm.15	satalink.13 	QNAP TS-101/201
re.15	viaide.13	IOMEGA StorCenter

- PCI line/pin and EPIC IRQ assignments

		PCI IDSEL	   EPIC IRQ
Kurobox		11, 12, 13, 14	-> 0, 1, 4, 3
Synology	12, 13, 14, 15	-> 4, 0, 1, 2
QNAP		12, 13, 14, 15	-> 0, 1, 2, 3
StorCenter	12, 13, 14, 15  -> 0, 1, 2, 3

- USB EHCI is a multiple function PCI device which has
  pin assignment A, B and C.

- *attention* Synology DS407 is a MPC8349E product.

Freescale MPC8349E-mITXE
		PCI IDSEL 	   MPC8349 IRQ
  1st PCI	16		-> 22
		satalink
  2nd PCI	14, 15		-> 21, 20
		miniPCI, 3.3VPCI

Synology DS407
		PCI IDSEL	   MPC8349 IRQ
  1st PCI	13, 14, 15, 16	-> 20, 21, 22, 19
		satalink,satalink,ehci,PCI
  2nd PCI	???

[ research still in progress ]

---

