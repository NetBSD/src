#ifndef _IBM4XX_IBM405GP_H_
#define	_IBM4XX_IBM405GP_H_

#define	PCIL_BASE	0xef400000
#define	PMM0LA		0x00
#define	PMM0MA		0x04
#define	PMM0PCILA	0x08
#define	PMM0PCIHA	0x0c
#define	PMM1LA		0x10
#define	PMM1MA		0x14
#define	PMM1PCILA	0x18
#define	PMM1PCIHA	0x1c
#define	PMM2LA		0x20
#define	PMM2MA		0x24
#define	PMM2PCILA	0x28
#define	PMM2PCIHA	0x2c
#define	PTM1MS		0x30
#define	PTM1LA		0x34
#define	PTM2MS		0x38
#define	PTM2LA		0x3c

void galaxy_show_pci_map(void);
void galaxy_setup_pci(void);
#endif	/* _IBM4XX_IBM405GP_H_ */
