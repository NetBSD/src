/* $NetBSD: lptio.h,v 1.3 2004/01/28 09:29:06 jdolecek Exp $ */

#ifndef __DEV_PPBUS_LPTIO_H_
#define __DEV_PPBUS_LPTIO_H_

/* Definitions for get status command */
enum lpt_mode_t {
	standard = 1,
	nibble = 2,
	ps2 = 3,
	fast = 4,
	ecp = 5,
	epp = 6
};

typedef struct {
	u_int16_t mode_status;
	u_int8_t dma_status;
	u_int8_t ieee_status;
} LPT_INFO_T;

/* LPT ioctl commands */
#define LPTIO_ENABLE_DMA	_IO('L', 0)
#define LPTIO_DISABLE_DMA	_IO('L', 1)
#define LPTIO_MODE_STD		_IO('L', 2)
#define LPTIO_MODE_FAST		_IO('L', 3)
#define LPTIO_MODE_PS2		_IO('L', 4)
#define LPTIO_MODE_ECP		_IO('L', 5)
#define LPTIO_MODE_EPP		_IO('L', 6)
#define LPTIO_MODE_NIBBLE	_IO('L', 7)
#define LPTIO_ENABLE_IEEE	_IO('L', 8)
#define LPTIO_DISABLE_IEEE	_IO('L', 9)
#define LPTIO_GET_STATUS	_IOR('L', 10, LPT_INFO_T)

#endif /* __DEV_PPBUS_LPTIO_H_ */
