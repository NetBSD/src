#ifndef _ARM_IMX_IMXI2CREG_H
#define	_ARM_IMX_IMXI2CREG_H

#define	I2C_IADR	0x00	/* I2C address register */
#define	I2C_IFDR	0x04	/* I2C frequency divider register */
#define	I2C_I2CR	0x08	/* I2C control register */
#define	 I2CR_IEN	0x80	/* I2C enable */
#define	 I2CR_IIEN	0x40	/* I2C interrupt enable */
#define	 I2CR_MSTA	0x20	/* Master/slave mode */
#define	 I2CR_MTX	0x10	/* Transmit/receive mode */
#define	 I2CR_TXAK	0x08	/* Transmit acknowledge */
#define	 I2CR_RSTA	0x04	/* Repeat start */
#define	I2C_I2SR	0x0c	/* I2C status register */
#define	 I2SR_ICF	0x80	/* Data transferring */
#define	 I2SR_IAAS	0x40	/* I2C addressed as a slave */
#define	 I2SR_IBB	0x20	/* I2C busy */
#define	 I2SR_IAL	0x10	/* I2C Arbitration lost */
#define	 I2SR_SRW	0x04	/* Slave read/write */
#define	 I2SR_IIF	0x02	/* I2C interrupt */
#define	 I2SR_RXAK	0x01	/* Received acknowledge */
#define	I2C_I2DR	0x10	/* I2C data I/O register */
#define	I2C_SIZE	0x4000

#endif	/* _ARM_IMX_IMXI2CREG_H */
