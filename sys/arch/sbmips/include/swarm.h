/* $NetBSD: swarm.h,v 1.1 2002/03/17 06:24:30 simonb Exp $ */

/*
 * I/O Address assignments for the CSWARM board
 *
 * Summary of address map:
 *
 * Address         Size   CSel    Description
 * --------------- ----   ------  --------------------------------
 * 0x1FC00000      2MB     CS0    Boot ROM
 * 0x1F800000      2MB     CS1    Alternate boot ROM
 *                         CS2    Unused
 * 0x100A0000      64KB    CS3    LED display
 * 0x100B0000      64KB    CS4    IDE Disk
 *                         CS5    Unused
 * 0x11000000      64MB    CS6    PCMCIA
 *                         CS7    Unused
 *
 * GPIO assignments
 *
 * GPIO#    Direction	Description
 * -------  ---------	------------------------------------------
 * GPIO0    Output	Debug LED
 * GPIO1    Output	Sturgeon NMI
 * GPIO2    Input	PHY Interrupt			(interrupt)
 * GPIO3    Input	Nonmaskable Interrupt		(interrupt)
 * GPIO4    Input	IDE Disk Interrupt		(interrupt)
 * GPIO5    Input	Temperature Sensor Alert	(interrupt)
 * GPIO6    N/A		PCMCIA interface
 * GPIO7    N/A		PCMCIA interface
 * GPIO8    N/A		PCMCIA interface
 * GPIO9    N/A		PCMCIA interface
 * GPIO10   N/A		PCMCIA interface
 * GPIO11   N/A		PCMCIA interface
 * GPIO12   N/A		PCMCIA interface
 * GPIO13   N/A		PCMCIA interface
 * GPIO14   N/A		PCMCIA interface
 * GPIO15   N/A		PCMCIA interface
*/


/* GPIO pins */
#define	GPIO_DEBUG_LED		0
#define	GPIO_STURGEON_NMI	1
#define	GPIO_PHY_INTERRUPT	2
#define	GPIO_NONMASKABLE_INT	3
#define	GPIO_IDE_INTERRUPT	4
#define	GPIO_TEMP_SENSOR_INT	5

/* device addresses */
#define	SWARM_LEDS_PHYS		0x100a0000
#define	SWARM_IDE_PHYS		0x100b0000
#define	SWARM_PCMCIA_PHYS	0x11000000

/* SMBus devices */
#define	TEMPSENSOR_SMBUS_CHAN	0
#define	TEMPSENSOR_SMBUS_DEV	0x2A

#define	DRAM_SMBUS_CHAN		0
#define	DRAM_SMBUS_DEV		0x54

#define	BIGEEPROM_SMBUS_CHAN_1	1
#define	BIGEEPROM_SMBUS_DEV_1	0x51

#define	BIGEEPROM_SMBUS_CHAN	0
#define	BIGEEPROM_SMBUS_DEV	0x50

#define	CFG_DRAM_SMBUS_CHAN	0
#define	CFG_DRAM_SMBUS_BASE	0x54	/* starting SMBus device base */

#define	X1240_SMBUS_CHAN	1
#define	X1240_ARRAY_SMBUS_DEV	0x57	/* SMBus address of EEPROM */
#define	X1240_RTC_SMBUS_DEV	0x6f	/* SMBus address of RTC */

#define	M41T81_SMBUS_CHAN	1	/* Only on rev 2.0 Swarm */
#define	M41T81_SMBUS_DEV	0x68	/* that does not have an X1240 */
