#define	IQ80310_UART1		0xfe800000UL
#define	UART1_LEN		8
#define	IQ80310_UART2		0xfe810000UL
#define	UART2_LEN		8
#define	IQ80310_XINT3STS	0xfe820000UL
#define	 XINT3STS_LEN		1
#define	 XINT3_TIMER		 0x01
#define	 XINT3_ETHERNET		 0x02
#define	 XINT3_UART1		 0x04
#define	 XINT3_UART2		 0x08
#define	 XINT3_SINTD		 0x10		/* Secondary PCI INTD */
#define	IQ80310_BRDREV		0xfe830000UL	/* rev F board feature */
#define	 BRDREV_LEN		1
#define	 BRDREV_REV(data)	((data) & 0xf) + '@')
#define	IQ80310_CPLDREV		0xfe840000UL	/* rev F board feature (R0) */
#define	 CPLDREV_LEN		1
#define	 CPLDREV_REV(data)	((data) & 0xf) + '@')
#define	IQ80310_SEGMENT1	0xfe840000UL	/* (WO) */
#define	 SEGMENT1_LEN		1
#define	 SEGMENT_A1		 0x01		/* top off */
#define	 SEGMENT_B1		 0x02		/* upper right off */
#define	 SEGMENT_C1		 0x04		/* lower right off */
#define	 SEGMENT_D1		 0x08		/* bottom off */
#define	 SEGMENT_E1		 0x10		/* lower left off */
#define	 SEGMENT_F1		 0x20		/* upper left off */
#define	 SEGMENT_G1		 0x40		/* middle off */
#define	 SEGMENT_DP1		 0x80		/* decimal point off */
#define	IQ80310_XINT0STS	0xfe850000UL	/* rev F board feature (RO) */
#define	 XINT0_LEN		1
#define	 XINT0_SINTA		 0x01		/* Secondary PCI INTA */
#define	 XINT0_SINTB		 0x02		/* Secondary PCI INTB */
#define	 XINT0_SINTC		 0x04		/* Secondary PCI INTC */
#define	IQ80310_SEGMENT2	0xfe850000UL	/* (WO) */
#define	 SEGMENT2_LEN		1
#define	IQ80310_XINT3MSK	0xfe860000UL	/* rev F board feature (RO) */
#define	 XINT3MSK_LEN		1
#define	IQ80310_BCKPLNDTCT	0xfe870000UL	/* (RO) */
#define	 BCKPLN_PRESENT		 0x01
