#ifndef _IBM4XX_MAL405GP_H_
#define	_IBM4XX_MAL405GP_H_

/* Memory Access Layer buffer descriptor */
struct mal_descriptor {
	volatile u_int16_t md_stat_ctrl;	/* Status/Control */
	volatile u_int16_t md_data_len;		/* Data Len (low 12 bits only) */
	volatile u_int32_t md_data;		/* Data pointer */
};

/* MAL transmit status/control definitions */
#define	MAL_TX_READY		0x8000
#define	MAL_TX_WRAP		0x4000
#define	MAL_TX_CONTINUOUS_MODE	0x2000
#define	MAL_TX_LAST		0x1000
#define	MAL_TX_INTERRUPT	0x0400

/* MAL receive status/control definitions */
#define	MAL_RX_EMPTY		0x8000
#define	MAL_RX_WRAP		0x4000
#define	MAL_RX_CONTINUOUS_MODE	0x2000
#define	MAL_RX_LAST		0x1000
#define	MAL_RX_FIRST		0x0800
#define	MAL_RX_INTERRUPT	0x0400

#endif /* _IBM4XX_MAL405GP_H_ */
