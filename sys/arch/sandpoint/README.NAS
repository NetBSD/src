$NetBSD: README.NAS,v 1.4.4.1 2010/08/11 22:52:38 yamt Exp $

//// MPC8241/8245 NAS products ////

The following NAS products are known by NetBSD/sandpoint.

NIC	IDE		machine description
----	----		--------------------------
tlp.11	cmdide.12	classic KuroBox, LinkStation HD-HLAN(LS1)
re.11	cmdide.12	KuroBox HG
re.11	iteide.12	Gigabit LinkStation HD-HGLAN
re.11	iteide.12/13	classic TeraStation HD-HTGL
re.11	satalink.12/13	TeraStation Pro TS-TGL v1
sk.15	iteide.13	Synology DS-106j, LinkStation LANxxxG
sk.15	satalink.13	Synology DS-101g+/106e/106/106x/107e/107/207
			DS-108j/109j/209j
sk.15	satalink.12/13	Synology CS-406/RS-406/CS-406e/CS-407e
wm.15	satalink.13 	QNAP TS-101/201
re.15	viaide.13	IOMEGA StorCenter

- PCI line/pin and EPIC IRQ assignments

		PCI IDSEL	   EPIC IRQ
Kurobox		11, 12, 13, 14	-> 0, 1, 4, 3
Synology	12, 13, 14, 15	-> 4, 0, 1, 2
QNAP		13, 14, 15, 16	-> 0, 1, 2, 3
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

