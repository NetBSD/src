#ifndef __DEV_PPBUS_LPTIO_H_
#define __DEV_PPBUS_LPTIO_H_

/* Definitions for get status command */
enum boolean_t { false, true};
enum mode_t { standard, nibble, ps2, fast, ecp, epp };

typedef struct {
	enum boolean_t dma_status;
	enum boolean_t ieee_status;
	enum mode_t mode_status;
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
