
#define IIO_CFLOC_ADDR(cf)	(IIOV(0xE00000 + (cf)->cf_loc[0] * 0x10000))
#define IIO_CFLOC_LEVEL(cf)	((cf)->cf_loc[1])
