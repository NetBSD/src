$NetBSD: README.NAS,v 1.22 2012/11/26 17:56:17 phx Exp $

//// MPC8241/8245 NAS products ////

The following NAS products are known by NetBSD/sandpoint.

NIC	IDE		machine description
----	----		--------------------------
			>> Board type BRD_KUROBOX <<
tlp.11	cmdide.12	classic KuroBox, LinkStation HD-HLAN(LS1)
re.11	cmdide.12	KuroBox HG
re.11	iteide.12	Gigabit LinkStation HD-HGLAN (also with cmdide)
re.11	iteide.12/13	classic TeraStation HD-HTGL
re.11	satalink.12/13	TeraStation Pro TS-TGL v1

			>> Board type BRD_SYNOLOGY <<
sk.15	iteide.13	Synology DS-106j, LinkStation LANxxxG
sk.15	satalink.13	Synology DS-101g+/106e/106/106x/107e/107/207
			DS-108j/109j/209j
sk.15	satalink.12/13	Synology CS-406/RS-406/CS-406e/CS-407e

			>> Board type BRD_QNAPTS <<
wm.15	satalink.13 	QNAP TS-100/TS-101(V1.02)
re.15	satalink.13	QNAP TS-101(V200), QNAP TS-201, LevelOne FNS-5000B

			>> Board type BRD_STORCENTER <<
re.15	viaide.13	IOMEGA StorCenter 250/500/1T

			>> Board type BRD_DLINKDSM <<
stge.15	acardide.16	D-Link DSM-G600 (Rev.B only!), Conceptronic CH3WNAS

			>> Board type BRD_NH230NAS <<
re.15	iteide.16	Netronix NH-230, Allnet ALL6250, Encore ENNHD-1000,
			Planex NAS-01G
re.15	satalink.16	Netronix NH-231, Allnet ALL6260, Longshine LCS-8311,
			Fujitsu-Siemens AMS150, Fujitsu-Siemens SBLAN2,
			Vibe NS-349-S


PCI line/pin and EPIC IRQ assignments

		PCI IDSEL	   EPIC IRQ
Kurobox		11, 12, 13, 14	-> 0, 1, 4, 3
Synology	12, 13, 14, 15	-> 4, 0, 1, 2
QNAP		13, 14, 15, 16	-> 0, 1, 2, 3
StorCenter	13, 14, 15      -> 1, *, 0
DLink		13, 14, 15, 16	-> 0, *, 3, 4
NH230		13, 14, 15, 16	-> 0, 3, 1, 2

- USB is a multiple function PCI device which has
  pin assignment A (OHCI), B (OHCI) and C (EHCI).
  Special cases (*):
		IDSEL	Pin	   EPIC IRQ
  DLink		14	A, B, C	-> 1, 1, 2
  StorCenter	14	A, B, C	-> 2, 3, 4


//// e300 NAS products ////

- Freescale MPC8349E-mITXE

  PCI		bus:IDSEL	IPIC IRQ
satalink	0:16		22
miniPCI		1:14		21
3.3vPCI		1:15		20

  I2C		bus:addr
ds1339 RTC	1:0x68
sat MCU		1:0x0a

- Thecus N1200/N3200 (MPC8347)

  PCI		IDSEL		IPIC IRQ
satalink	16		20

  I2C		bus:addr
f75375 fanctl	0:0x2e
rs5c372a RTC	0:0x32
sat MCU		0:0x36


[ research still in progress ]

---
