/*-
 * Copyright (c) 2017 Mediatek Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/condvar.h>
#include <arm/mediatek/mercury_reg.h>
#include <dev/fdt/fdtvar.h>
#include <dev/i2c/i2cvar.h>

#define MTK_I2C_SOURCE_CLK 68250
#define MTK_I2C_CLK_DIV 1

#define MTK_GPIO_I2C0_SET 0xB0
#define MTK_GPIO_I2C1_SET 0xA0
#define MTK_GPIO_I2C2_SET 0xC0

#define MTK_GPIO_SDA0 9
#define MTK_GPIO_SCL0 12
#define MTK_GPIO_SDA1 6
#define MTK_GPIO_SCL1 9
#define MTK_GPIO_SDA2 0
#define MTK_GPIO_SCL2 3

#define MTK_I2C_INFRACFG 0x1080
#define MTK_I2C_CLK_SET 0x54
#define MTK_I2C_CLK_CLR 0x84
#define MTK_I2C0_CLK_OFFSET (0x1 << 3)
#define MTK_I2C1_CLK_OFFSET (0x1 << 4)
#define MTK_I2C2_CLK_OFFSET (0x1 << 16)
#define MTK_APDMA_CLK_OFFSET (0x1 << 2)

#define I2C_CONTROL_RS			(0x1 << 1)
#define I2C_CONTROL_DMA_EN		(0x1 << 2)
#define I2C_CONTROL_CLK_EXT_EN		(0x1 << 3)
#define I2C_CONTROL_DIR_CHANGE		(0x1 << 4)
#define I2C_CONTROL_ACKERR_DET_EN	(0x1 << 5)
#define I2C_CONTROL_TRANSFER_LEN_CHANGE	(0x1 << 6)
#define I2C_CONTROL_WRAPPER		(0x1 << 0)

#define I2C_RS_TRANSFER			(1 << 4)
#define I2C_HS_NACKERR			(1 << 2)
#define I2C_ACKERR			(1 << 1)
#define I2C_TRANSAC_COMP		(1 << 0)
#define I2C_TRANSAC_START		(1 << 0)
#define I2C_RS_MUL_CNFG			(1 << 15)
#define I2C_RS_MUL_TRIG			(1 << 14)

#define I2C_DCM_DISABLE			0x0000
#define I2C_IO_CONFIG_OPEN_DRAIN	0x0003
#define I2C_IO_CONFIG_PUSH_PULL		0x0000
#define I2C_SOFT_RST			0x0001
#define I2C_FIFO_ADDR_CLR		0x0001
#define I2C_DELAY_LEN			0x0002
#define I2C_ST_START_CON		0x8001
#define I2C_FS_START_CON		0x1800
#define I2C_TIME_CLR_VALUE		0x0000
#define I2C_TIME_DEFAULT_VALUE		0x0003
#define I2C_WRRD_TRANAC_VALUE		0x0002
#define I2C_RD_TRANAC_VALUE		0x0001
#define I2C_DMA_CON_TX			0x0000
#define I2C_DMA_CON_RX			0x0001
#define I2C_DMA_START_EN		0x0001
#define I2C_DMA_INT_FLAG_NONE		0x0000
#define I2C_DMA_CLR_FLAG		0x0000
#define I2C_DMA_HARD_RST		0x0002
#define I2C_DMA_4G_MODE			0x0001

#define I2C_FIFO_SIZE			8
#define I2C_DEFAULT_CLK_DIV		5
#define MAX_ST_MODE_SPEED		100
#define MAX_FS_MODE_SPEED		400
#define MAX_HS_MODE_SPEED		3400
#define MAX_SAMPLE_CNT_DIV		8
#define MAX_STEP_CNT_DIV		64
#define MAX_HS_STEP_CNT_DIV		8

#define I2C_HS_NACKERR			(1 << 2)
#define I2C_ACKERR			(1 << 1)
#define I2C_TRANSAC_COMP		(1 << 0)

#define I2C_M_RD			0x0001  /* read data, from slave to master */
#define I2C_OK				0

#define DIV_ROUND_UP(x,y) (((x) + ((y) - 1)) / (y))

#define I2C_READ(sc, reg) \
	bus_space_read_2((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define I2C_WRITE(sc, reg, val) \
	bus_space_write_2((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

#define I2C_GPIO_READ(sc, reg) \
	bus_space_read_2((sc)->sc_bst, (sc)->sc_gpiobsh, (reg))
#define I2C_GPIO_WRITE(sc, reg, val) \
	bus_space_write_2((sc)->sc_bst, (sc)->sc_gpiobsh, (reg), (val))

#define I2C_CLK_READ(sc, reg) \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_clkbsh, (reg))
#define I2C_CLK_WRITE(sc, reg, val) \
	bus_space_write_4((sc)->sc_bst, (sc)->sc_clkbsh, (reg), (val))

#define I2C_DMA_READ(sc, reg) \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_dmabsh, (reg))
#define I2C_DMA_WRITE(sc, reg, val) \
	bus_space_write_4((sc)->sc_bst, (sc)->sc_dmabsh, (reg), (val))

enum I2C_REGS_OFFSET {
	OFFSET_DATA_PORT		= 0x0,
	OFFSET_SLAVE_ADDR		= 0x04,
	OFFSET_INTR_MASK		= 0x08,
	OFFSET_INTR_STAT		= 0x0C,
	OFFSET_CONTROL			= 0x10,
	OFFSET_TRANSFER_LEN		= 0x14,
	OFFSET_TRANSAC_LEN		= 0x18,
	OFFSET_DELAY_LEN		= 0x1C,
	OFFSET_TIMING			= 0x20,
	OFFSET_START			= 0x24,
	OFFSET_EXT_CONF			= 0x28,
	OFFSET_FIFO_STAT		= 0x30,
	OFFSET_FIFO_THRESH		= 0x34,
	OFFSET_FIFO_ADDR_CLR		= 0x38,
	OFFSET_IO_CONFIG		= 0x40,
	OFFSET_RSV_DEBUG		= 0x44,
	OFFSET_HS			= 0x48,
	OFFSET_SOFTRESET		= 0x50,
	OFFSET_DCM_EN			= 0x54,
	OFFSET_DEBUGSTAT		= 0x64,
	OFFSET_DEBUGCTRL		= 0x68,
	OFFSET_TRANSFER_LEN_AUX		= 0x6C,
	OFFSET_CLOCK_DIV		= 0x70,
	OFFSET_SCL_HL_RATIO		= 0x74,
	OFFSET_SCL_HS_HL_RATIO		= 0x78,
	OFFSET_SCL_MIS_COMP_POINT	= 0x7C,
	OFFSET_STA_STOP_AC_TIME		= 0x80,
	OFFSET_HS_STA_STOP_AC_TIME	= 0x84,
	OFFSET_DATA_TIME		= 0x88,
};

enum DMA_REGS_OFFSET {
	OFFSET_INT_FLAG		= 0x0,
	OFFSET_INT_EN		= 0x04,
	OFFSET_EN		= 0x08,
	OFFSET_RST		= 0x0C,
	OFFSET_CON		= 0x18,
	OFFSET_TX_MEM_ADDR	= 0x1C,
	OFFSET_RX_MEM_ADDR	= 0x20,
	OFFSET_TX_LEN		= 0x24,
	OFFSET_RX_LEN		= 0x28,
};

enum mt_trans_op {
	I2C_MASTER_NONE = 0,
	I2C_MASTER_WR = 1,
	I2C_MASTER_RD,
	I2C_MASTER_WRRD,
};

struct i2c_msg {
	uint16_t addr;		/* slave address */
	uint16_t flags;
	uint16_t len;		/* msg length */
	uint8_t *buf;		/* pointer to msg data */
};

struct mt_i2c {
	bool   dma_en;
	bool   filter_msg;	/* filter msg error log */
	bool   auto_restart;
	bool   aux_len_reg;
	uint16_t id;
	uint16_t irq_stat;	/* i2c interrupt status */
	uint16_t timing_reg;
	uint16_t high_speed_reg;
	uint32_t clk;		/* source clock khz */
	uint32_t clk_src_div;
	uint32_t speed;		/* khz */
	enum mt_trans_op op;
};

struct mtk_i2c_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	bus_space_handle_t sc_gpiobsh;
	bus_space_handle_t sc_clkbsh;
	bus_space_handle_t sc_dmabsh;
	struct i2c_controller sc_ic;
	kmutex_t sc_lock;
	kcondvar_t sc_cv;
	device_t sc_i2cdev;
	void *sc_ih;
	struct mt_i2c i2c;
};

static int mtk_i2c_acquire_bus(void *, int);
static void mtk_i2c_release_bus(void *, int);
static int mtk_i2c_exec(void *, i2c_op_t, i2c_addr_t, const void *,
			size_t, void *, size_t, int);
static int mtk_i2c_intr(void *);
static int mtk_i2c_match(device_t, cfdata_t, void *);
static void mtk_i2c_attach(device_t, device_t, void *);

static const char * const compatible[] = {
	"mediatek,mercury-i2c",
	NULL
};

CFATTACH_DECL_NEW(mtk_i2c, sizeof(struct mtk_i2c_softc),
		  mtk_i2c_match, mtk_i2c_attach, NULL, NULL);

static int mtk_i2c_clock_enable(struct mtk_i2c_softc *sc)
{
	struct mt_i2c *i2c = &(sc->i2c);
	uint32_t clk_reg;

	clk_reg = I2C_CLK_READ(sc, MTK_I2C_INFRACFG) & (~0xe);
	I2C_CLK_WRITE(sc, MTK_I2C_INFRACFG, clk_reg);

	switch (i2c->id) {
		case 0:
			I2C_CLK_WRITE(sc, MTK_I2C_CLK_CLR, MTK_I2C0_CLK_OFFSET);
			break;
		case 1:
			I2C_CLK_WRITE(sc, MTK_I2C_CLK_CLR, MTK_I2C1_CLK_OFFSET);
			break;
		case 2:
			I2C_CLK_WRITE(sc, MTK_I2C_CLK_CLR, MTK_I2C2_CLK_OFFSET);
			break;
		default:
			printf("[I2C]i2c clk enable, invalid para: i2c->id=%d\n", i2c->id);
			return EINVAL;
	}

	I2C_CLK_WRITE(sc, MTK_I2C_CLK_CLR, MTK_APDMA_CLK_OFFSET);

	return 0;
}

static int mtk_i2c_clock_disable(struct mtk_i2c_softc *sc)
{
	struct mt_i2c *i2c = &(sc->i2c);

	switch (i2c->id) {
		case 0:
			I2C_CLK_WRITE(sc, MTK_I2C_CLK_SET, MTK_I2C0_CLK_OFFSET);
			break;
		case 1:
			I2C_CLK_WRITE(sc, MTK_I2C_CLK_SET, MTK_I2C1_CLK_OFFSET);
			break;
		case 2:
			I2C_CLK_WRITE(sc, MTK_I2C_CLK_SET, MTK_I2C2_CLK_OFFSET);
			break;
		default:
			printf("[I2C]i2c clk disable, invalid para: i2c->id=%d\n", i2c->id);
			return EINVAL;
	}

	I2C_CLK_WRITE(sc, MTK_I2C_CLK_SET, MTK_APDMA_CLK_OFFSET);

	return 0;
}

static int mtk_i2c_gpio_set(struct mtk_i2c_softc *sc)
{
	uint16_t gpio_reg;
	struct mt_i2c *i2c = &(sc->i2c);

	switch (i2c->id) {
		case 0:
			gpio_reg = (I2C_GPIO_READ(sc, MTK_GPIO_I2C0_SET) & (~(0x7 << MTK_GPIO_SDA0)))
				   | (0x1 << MTK_GPIO_SDA0);
			I2C_GPIO_WRITE(sc, MTK_GPIO_I2C0_SET, gpio_reg);
			gpio_reg = (I2C_GPIO_READ(sc, MTK_GPIO_I2C0_SET) & (~(0x7 << MTK_GPIO_SCL0)))
				   | (0x1 << MTK_GPIO_SCL0);
			I2C_GPIO_WRITE(sc, MTK_GPIO_I2C0_SET, gpio_reg);
			break;
		case 1:
			gpio_reg = (I2C_GPIO_READ(sc, MTK_GPIO_I2C1_SET) & (~(0x7 << MTK_GPIO_SDA1)))
				   | (0x1 << MTK_GPIO_SDA1);
			I2C_GPIO_WRITE(sc, MTK_GPIO_I2C1_SET, gpio_reg);
			gpio_reg = (I2C_GPIO_READ(sc, MTK_GPIO_I2C1_SET) & (~(0x7 << MTK_GPIO_SCL1)))
				   | (0x1 << MTK_GPIO_SCL1);
			I2C_GPIO_WRITE(sc, MTK_GPIO_I2C1_SET, gpio_reg);
			break;
		case 2:
			gpio_reg = (I2C_GPIO_READ(sc, MTK_GPIO_I2C2_SET) & (~(0x7 << MTK_GPIO_SDA2)))
				   | (0x1 << MTK_GPIO_SDA2);
			I2C_GPIO_WRITE(sc, MTK_GPIO_I2C2_SET, gpio_reg);
			gpio_reg = (I2C_GPIO_READ(sc, MTK_GPIO_I2C2_SET) & (~(0x7 << MTK_GPIO_SCL2)))
				   | (0x1 << MTK_GPIO_SCL2);
			I2C_GPIO_WRITE(sc, MTK_GPIO_I2C2_SET, gpio_reg);
			break;
		default:
			printf("[I2C]i2c clk enable, invalid para: i2c->id=%d\n",i2c->id);
			return EINVAL;
	}

	return 0;
}

static int mtk_i2c_init_base(struct mtk_i2c_softc *sc)
{
	struct mt_i2c *i2c = &(sc->i2c);
	int ret;

	ret = mtk_i2c_gpio_set(sc);
	if (ret != 0)
		return ret;

	i2c->clk = MTK_I2C_SOURCE_CLK;
	i2c->clk_src_div = MTK_I2C_CLK_DIV;

	i2c->aux_len_reg = true;

	return 0;
}

static void mtk_i2c_init_hw(struct mtk_i2c_softc *sc)
{
	struct mt_i2c *i2c = &(sc->i2c);

	uint16_t control_reg;

	I2C_WRITE(sc, OFFSET_SOFTRESET, I2C_SOFT_RST);

	/* Set ioconfig */
	I2C_WRITE(sc, OFFSET_IO_CONFIG, I2C_IO_CONFIG_OPEN_DRAIN);

	I2C_WRITE(sc, OFFSET_DCM_EN, I2C_DCM_DISABLE);

	I2C_WRITE(sc, OFFSET_CLOCK_DIV, (i2c->clk_src_div - 1));

	I2C_WRITE(sc, OFFSET_TIMING, i2c->timing_reg);
	I2C_WRITE(sc, OFFSET_HS, i2c->high_speed_reg);

	control_reg = I2C_CONTROL_ACKERR_DET_EN |
		      I2C_CONTROL_CLK_EXT_EN | I2C_CONTROL_DMA_EN;
	I2C_WRITE(sc, OFFSET_CONTROL, control_reg);
	I2C_WRITE(sc, OFFSET_DELAY_LEN, I2C_DELAY_LEN);

	I2C_DMA_WRITE(sc, OFFSET_RST, I2C_DMA_HARD_RST);
	delay(50);
	I2C_DMA_WRITE(sc, OFFSET_RST, I2C_DMA_CLR_FLAG);
}

/*
 * Calculate i2c port speed
 *
 * Hardware design:
 * i2c_bus_freq = parent_clk / (clock_div * 2 * sample_cnt * step_cnt)
 * clock_div: fixed in hardware, but may be various in different SoCs
 *
 * The calculation want to pick the highest bus frequency that is still
 * less than or equal to i2c->speed_hz. The calculation try to get
 * sample_cnt and step_cn
 */
static int mtk_i2c_calculate_speed(unsigned int clk_src,
				   unsigned int target_speed,
				   unsigned int *timing_step_cnt,
				   unsigned int *timing_sample_cnt)
{
	unsigned int step_cnt;
	unsigned int sample_cnt;
	unsigned int max_step_cnt;
	unsigned int base_sample_cnt = MAX_SAMPLE_CNT_DIV;;
	unsigned int base_step_cnt;
	unsigned int opt_div;
	unsigned int best_mul;
	unsigned int cnt_mul;

	if (target_speed > MAX_HS_MODE_SPEED)
		target_speed = MAX_HS_MODE_SPEED;

	if (target_speed > MAX_FS_MODE_SPEED)
		max_step_cnt = MAX_HS_STEP_CNT_DIV;
	else
		max_step_cnt = MAX_STEP_CNT_DIV;

	base_step_cnt = max_step_cnt;

	/* find the best combination */
	opt_div = DIV_ROUND_UP(clk_src >> 1, target_speed);
	best_mul = MAX_SAMPLE_CNT_DIV * max_step_cnt;

	/* Search for the best pair (sample_cnt, step_cnt) with
	 * 0 < sample_cnt < MAX_SAMPLE_CNT_DIV
	 * 0 < step_cnt < max_step_cnt
	 * sample_cnt * step_cnt >= opt_div
	 * optimizing for sample_cnt * step_cnt being minimal
	 */
	for (sample_cnt = 1; sample_cnt <= MAX_SAMPLE_CNT_DIV; sample_cnt++) {
		step_cnt = DIV_ROUND_UP(opt_div, sample_cnt);
		cnt_mul = step_cnt * sample_cnt;
		if (step_cnt > max_step_cnt)
			continue;

		if (cnt_mul < best_mul) {
			best_mul = cnt_mul;
			base_sample_cnt = sample_cnt;
			base_step_cnt = step_cnt;
			if (best_mul == opt_div)
				break;
		}
	}

	sample_cnt = base_sample_cnt;
	step_cnt = base_step_cnt;

	if ((clk_src / (2 * sample_cnt * step_cnt)) > target_speed) {
		/* In this case, hardware can't support such
		 * low i2c_bus_freq
		 */
		printf("[I2C]Unsupported speed (%u khz)\n", target_speed);
		return EINVAL;
	}

	*timing_step_cnt = step_cnt - 1;
	*timing_sample_cnt = sample_cnt - 1;

	return 0;
}

static int mtk_i2c_set_speed(struct mtk_i2c_softc *sc)
{
	unsigned int clk_src;
	unsigned int step_cnt;
	unsigned int sample_cnt;
	unsigned int target_speed;
	int ret;
	struct mt_i2c *i2c = &(sc->i2c);

	if (i2c->speed == 0)
		i2c->speed = MAX_ST_MODE_SPEED;

	if (i2c->clk_src_div == 0)
		i2c->clk_src_div = MTK_I2C_CLK_DIV;

	i2c->clk_src_div *= I2C_DEFAULT_CLK_DIV;

	clk_src = (i2c->clk) / (i2c->clk_src_div);
	target_speed = i2c->speed;

	if (target_speed > MAX_FS_MODE_SPEED) {
		/* set master code speed register */
		ret = mtk_i2c_calculate_speed(clk_src, MAX_FS_MODE_SPEED,
					      &step_cnt, &sample_cnt);
		if (ret != 0)
			return ret;

		i2c->timing_reg = (sample_cnt << 8) | step_cnt;

		/* set the high speed mode register */
		ret = mtk_i2c_calculate_speed(clk_src, target_speed,
					      &step_cnt, &sample_cnt);
		if (ret != 0)
			return ret;

		i2c->high_speed_reg = I2C_TIME_DEFAULT_VALUE |
				      (sample_cnt << 12) |
				      (step_cnt << 8);
	} else {
		ret = mtk_i2c_calculate_speed(clk_src, target_speed,
					      &step_cnt, &sample_cnt);
		if (ret != 0)
			return ret;

		i2c->timing_reg = (sample_cnt << 8) | step_cnt;

		/* disable the high speed transaction */
		i2c->high_speed_reg = I2C_TIME_CLR_VALUE;
	}

	I2C_WRITE(sc, OFFSET_TIMING, i2c->timing_reg);
	I2C_WRITE(sc, OFFSET_HS, i2c->high_speed_reg);

	return 0;
}

static void i2c_dump_info(struct mtk_i2c_softc *sc)
{
	struct mt_i2c *i2c = &(sc->i2c);

	printf("[I2C]I2C structure:\n"
	       "[I2C]Id=%d,Dma_en=%x,Auto_restart=%x,Op=%x\n"
	       "[I2C]Irq_stat=%x,source_clk=%d,clk_div=%d,speed=%d\n",
	       i2c->id,i2c->dma_en,i2c->auto_restart,i2c->op,
	       i2c->irq_stat,i2c->clk,i2c->clk_src_div,i2c->speed);

	printf("[I2C]I2C register:\n"
	       "[I2C]SLAVE_ADDR=%x,INTR_MASK=%x,INTR_STAT=%x,CONTROL=%x\n"
	       "[I2C]TRANSFER_LEN=%x,TRANSAC_LEN=%x,DELAY_LEN=%x\n"
	       "[I2C]TIMING=%x,START=%x,FIFO_STAT=%x,IO_CONFIG=%x,HS=%x\n"
	       "[I2C]DCM_EN=%x,DEBUGSTAT=%x,EXT_CONF=%x\n"
	       "[I2C]TRANSFER_LEN_AUX=%x,FIFO_THRESH=%x,RSV_DEBUG=%x\n"
	       "[I2C]DEBUGCTRL=%x,CLOCK_DIV=%x,SCL_HL_RATIO=%x\n"
	       "[I2C]SCL_HS_HL_RATIO=%x,SCL_MIS_COMP_POINT=%x\n"
	       "[I2C]STA_STOP_AC_TIME=%x,HS_STA_STOP_AC_TIME=%x\n"
	       "[I2C]DATA_TIME=%x\n",
	       (I2C_READ(sc, OFFSET_SLAVE_ADDR)),
	       (I2C_READ(sc, OFFSET_INTR_MASK)),
	       (I2C_READ(sc, OFFSET_INTR_STAT)),
	       (I2C_READ(sc, OFFSET_CONTROL)),
	       (I2C_READ(sc, OFFSET_TRANSFER_LEN)),
	       (I2C_READ(sc, OFFSET_TRANSAC_LEN)),
	       (I2C_READ(sc, OFFSET_DELAY_LEN)),
	       (I2C_READ(sc, OFFSET_TIMING)),
	       (I2C_READ(sc, OFFSET_START)),
	       (I2C_READ(sc, OFFSET_FIFO_STAT)),
	       (I2C_READ(sc, OFFSET_IO_CONFIG)),
	       (I2C_READ(sc, OFFSET_HS)),
	       (I2C_READ(sc, OFFSET_DCM_EN)),
	       (I2C_READ(sc, OFFSET_DEBUGSTAT)),
	       (I2C_READ(sc, OFFSET_EXT_CONF)),
	       (I2C_READ(sc, OFFSET_TRANSFER_LEN_AUX)),
	       (I2C_READ(sc, OFFSET_FIFO_THRESH)),
	       (I2C_READ(sc, OFFSET_RSV_DEBUG)),
	       (I2C_READ(sc, OFFSET_DEBUGCTRL)),
	       (I2C_READ(sc, OFFSET_CLOCK_DIV)),
	       (I2C_READ(sc, OFFSET_SCL_HL_RATIO)),
	       (I2C_READ(sc, OFFSET_SCL_HS_HL_RATIO)),
	       (I2C_READ(sc, OFFSET_SCL_MIS_COMP_POINT)),
	       (I2C_READ(sc, OFFSET_STA_STOP_AC_TIME)),
	       (I2C_READ(sc, OFFSET_HS_STA_STOP_AC_TIME)),
	       (I2C_READ(sc, OFFSET_DATA_TIME)));
}


static int mtk_i2c_do_transfer(struct mtk_i2c_softc *sc, struct i2c_msg *msgs,
			       int num, int left_num, int flags)
{
	uint16_t addr_reg;
	uint16_t start_reg;
	uint16_t control_reg;
	uint16_t restart_flag = 0;
	uint8_t *data_point = msgs->buf;
	uint16_t data_size = msgs->len;
	int error = 0;
	int retry = 1;
	struct mt_i2c *i2c = &(sc->i2c);

	i2c->irq_stat = 0;

	if (i2c->auto_restart)
		restart_flag = I2C_RS_TRANSFER;

	control_reg = I2C_READ(sc, OFFSET_CONTROL) &
		      ~(I2C_CONTROL_DIR_CHANGE | I2C_CONTROL_RS);

	if ((i2c->speed > MAX_FS_MODE_SPEED) || (num > 1))
		control_reg |= I2C_CONTROL_RS;

	if (i2c->op == I2C_MASTER_WRRD)
		control_reg |= I2C_CONTROL_DIR_CHANGE | I2C_CONTROL_RS;

	if (i2c->dma_en)
		control_reg |= I2C_CONTROL_DMA_EN;

	I2C_WRITE(sc, OFFSET_CONTROL, control_reg);

	/* set start condition */
	if (i2c->speed <= MAX_ST_MODE_SPEED)
		I2C_WRITE(sc, OFFSET_EXT_CONF, I2C_ST_START_CON);
	else
		I2C_WRITE(sc, OFFSET_EXT_CONF, I2C_FS_START_CON);

	addr_reg = msgs->addr << 1;
	if (i2c->op == I2C_MASTER_RD)
		addr_reg |= 0x1;

	I2C_WRITE(sc, OFFSET_SLAVE_ADDR, addr_reg);

	/* clear interrupt status */
	I2C_WRITE(sc, OFFSET_INTR_STAT, restart_flag | I2C_HS_NACKERR |
		  I2C_ACKERR | I2C_TRANSAC_COMP);
	I2C_WRITE(sc, OFFSET_FIFO_ADDR_CLR, I2C_FIFO_ADDR_CLR);

	/* enable interrupt */
	I2C_WRITE(sc, OFFSET_INTR_MASK, restart_flag | I2C_HS_NACKERR |
		  I2C_ACKERR | I2C_TRANSAC_COMP);

	/* set transfer and transaction len */
	if (i2c->op == I2C_MASTER_WRRD) {
		if (i2c->aux_len_reg) {
			I2C_WRITE(sc, OFFSET_TRANSFER_LEN, msgs->len);
			I2C_WRITE(sc, OFFSET_TRANSFER_LEN_AUX, (msgs + 1)->len);
		} else {
			I2C_WRITE(sc, OFFSET_TRANSFER_LEN,
				  (msgs->len | (((msgs + 1)->len) << 8)));
		}
		I2C_WRITE(sc, OFFSET_TRANSAC_LEN, I2C_WRRD_TRANAC_VALUE);
	} else {
		I2C_WRITE(sc, OFFSET_TRANSFER_LEN, msgs->len);
		I2C_WRITE(sc, OFFSET_TRANSAC_LEN, num);
	}

	if (i2c->dma_en) {

	} else {
		if (I2C_MASTER_RD != i2c->op) {
			while (data_size--) {
				I2C_WRITE(sc, OFFSET_DATA_PORT, *data_point);
				data_point++;
			}
		}
	}

	if (!i2c->auto_restart) {
		start_reg = I2C_TRANSAC_START;
	} else {
		start_reg = I2C_TRANSAC_START | I2C_RS_MUL_TRIG;
		if (left_num >= 1)
			start_reg |= I2C_RS_MUL_CNFG;
	}

	I2C_WRITE(sc, OFFSET_START, start_reg);

	/* Wait up to 5 seconds for a transfer to complete */
	for (retry = (flags & I2C_F_POLL) ? 100 : 2; retry > 0; retry--) {
		if (flags & I2C_F_POLL) {
			i2c->irq_stat = I2C_READ(sc, OFFSET_INTR_STAT);
		} else {
			error = cv_timedwait(&sc->sc_cv, &sc->sc_lock, hz);
			if (error && error != EWOULDBLOCK) {
				break;
			}
		}
		if (i2c->irq_stat & (I2C_TRANSAC_COMP | restart_flag)) {
			break;
		}
		if (flags & I2C_F_POLL) {
			delay(10000);
		}
	}
	if (retry == 0)
		error = EAGAIN;

	/* clear interrupt mask */
	I2C_WRITE(sc, OFFSET_INTR_MASK, ~(restart_flag | I2C_HS_NACKERR |
		  I2C_ACKERR | I2C_TRANSAC_COMP));

	if (error) {
		aprint_error_dev(sc->sc_dev, "id: %d, addr: %x, transfer timeout, error = %d\n",
				 i2c->id, msgs->addr, error);
		i2c_dump_info(sc);
		mtk_i2c_init_hw(sc);
		return error;
	}

	if (i2c->irq_stat & (I2C_HS_NACKERR | I2C_ACKERR)) {
		aprint_error_dev(sc->sc_dev, "id: %d, addr: %x, transfer ACK error\n",
				 i2c->id, msgs->addr);
		i2c_dump_info(sc);
		mtk_i2c_init_hw(sc);
		return EIO;
	}

	/* transfer success ,we need to get data from fifo */
	if ((i2c->op == I2C_MASTER_RD) || (i2c->op == I2C_MASTER_WRRD)) {
		data_point = (i2c->op == I2C_MASTER_RD) ?
			     msgs->buf : (msgs + 1)->buf;

		if (!i2c->dma_en) {
			data_size = (I2C_READ(sc, OFFSET_FIFO_STAT) >> 4)
				    & 0x000F;

			while (data_size--) {
				*data_point = I2C_READ(sc, OFFSET_DATA_PORT);
				data_point++;
			}
		} else {

		}
	}

	return 0;
}

static int mtk_i2c_transfer(struct mtk_i2c_softc *sc,
			    struct i2c_msg *msgs, int num, int flags)
{
	int ret;
	int left_num = num;
	uint8_t num_cnt;
	struct mt_i2c *i2c = &(sc->i2c);

	ret = mtk_i2c_init_base(sc);
	if (ret != 0) {
		printf("[I2C]Failed to init i2c base.\n");
		return ret;
	}

	mtk_i2c_clock_enable(sc);

	ret = mtk_i2c_set_speed(sc);
	if (ret != 0) {
		printf("[I2C]Failed to set the speed.\n");
		goto err_exit;
	}

	mtk_i2c_init_hw(sc);

	for (num_cnt = 0; num_cnt < num; num_cnt++) {
		if (((msgs+num_cnt)->addr) > 0x7f) {
			printf("[I2C]i2c addr: msgs[%d]->addr(%x) > 0x7f, error! \n",
			       num_cnt, ((msgs+num_cnt)->addr));
			ret = EINVAL;
			goto err_exit;
		}

		if (((msgs+num_cnt)->len == 0) || ((msgs+num_cnt)->len > 8)) {
			printf("[I2C]FIFO MODE: msgs[%d]->len(%d) == 0 || > 8, error! \n",
			       num_cnt, ((msgs+num_cnt)->len));
			ret = EINVAL;
			goto err_exit;
		}

		if ((i2c->dma_en) && (((msgs+num_cnt)->len) > 255)) {
			printf("[I2C]DMA MODE: msgs[%d]->len(%d) > 255, error! \n",
			       num_cnt, ((msgs+num_cnt)->len));
			ret = EINVAL;
			goto err_exit;
		}
	}

	while (left_num--) {
		if (!msgs->buf) {
			printf("[I2C]data buffer is NULL.\n");
			ret = EINVAL;
			goto err_exit;
		}

		if (msgs->flags & I2C_M_RD)
			i2c->op = I2C_MASTER_RD;
		else
			i2c->op = I2C_MASTER_WR;

		if (!i2c->auto_restart) {
			if (num > 1) {
				/* combined two messages into one transaction */
				i2c->op = I2C_MASTER_WRRD;
				left_num--;
			}
		}

		ret = mtk_i2c_do_transfer(sc, msgs, num, left_num, flags);
		if (ret != 0)
			goto err_exit;

		msgs++;
	}

	ret = I2C_OK;

err_exit:
	mtk_i2c_clock_disable(sc);

	return ret;
}

static int mtk_i2c_intr(void *priv)
{
	struct mtk_i2c_softc *sc = priv;
	struct mt_i2c *i2c = &(sc->i2c);

	uint16_t restart_flag = 0;
	uint16_t intr_stat;

	if (i2c->auto_restart)
		restart_flag = I2C_RS_TRANSFER;

	intr_stat = I2C_READ(sc, OFFSET_INTR_STAT);
	I2C_WRITE(sc, OFFSET_INTR_STAT, intr_stat);

	/*
	 * when occurs ack error, i2c controller generate two interrupts
	 * first is the ack error interrupt, then the complete interrupt
	 * i2c->irq_stat need keep the two interrupt value.
	 */
	mutex_enter(&sc->sc_lock);
	i2c->irq_stat |= intr_stat;
	if (i2c->irq_stat & (I2C_TRANSAC_COMP | restart_flag))
		cv_broadcast(&sc->sc_cv);
	mutex_exit(&sc->sc_lock);

	return 1;
}

static int mtk_i2c_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void mtk_i2c_attach(device_t parent, device_t self, void *aux)
{
	struct mtk_i2c_softc *sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	struct i2cbus_attach_args iba;
	struct mt_i2c i2c;
	bus_addr_t clk_addr = 0x10000000;
	bus_size_t clk_size = 0x2000;
	bus_addr_t gpio_addr = 0x10005300;
	bus_size_t gpio_size = 0x100;
	char intrstr[128];
	bus_addr_t addr;
	bus_size_t size;

	memset(&i2c, 0, sizeof(i2c));
	i2c.id = self->dv_unit;
	memcpy(&(sc->i2c), &i2c, sizeof(i2c));
	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_SCHED);
	cv_init(&sc->sc_cv, "mtki2c");

	if (fdtbus_get_reg(faa->faa_phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error_dev(sc->sc_dev, "unable to map device\n");
		return;
	}

	if (fdtbus_get_reg(faa->faa_phandle, 1, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_dmabsh) != 0) {
		aprint_error_dev(sc->sc_dev, "unable to map device\n");
		return;
	}

	if (bus_space_map(sc->sc_bst, clk_addr, clk_size, 0, &sc->sc_clkbsh) != 0) {
		aprint_error_dev(sc->sc_dev, "unable to map device\n");
		return;
	}

	if (bus_space_map(sc->sc_bst, gpio_addr, gpio_size, 0, &sc->sc_gpiobsh) != 0) {
		aprint_error_dev(sc->sc_dev, "unable to map device\n");
		return;
	}

	if (!fdtbus_intr_str(faa->faa_phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(sc->sc_dev, "failed to decode interrupt\n");
		return;
	}

	sc->sc_ih = fdtbus_intr_establish(faa->faa_phandle, 0, IPL_BIO,
		FDT_INTR_MPSAFE, mtk_i2c_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(sc->sc_dev, "failed to establish interrupt on %s\n",
			intrstr);
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	sc->sc_ic.ic_cookie = sc;
	sc->sc_ic.ic_acquire_bus = mtk_i2c_acquire_bus;
	sc->sc_ic.ic_release_bus = mtk_i2c_release_bus;
	sc->sc_ic.ic_exec = mtk_i2c_exec;

	memset(&iba, 0, sizeof(iba));
	iba.iba_tag = &sc->sc_ic;
	sc->sc_i2cdev = config_found_ia(self, "i2cbus", &iba, iicbus_print);
}

static int mtk_i2c_acquire_bus(void *priv, int flags)
{
	struct mtk_i2c_softc *sc = priv;

	if (flags & I2C_F_POLL) {
		if (!mutex_tryenter(&sc->sc_lock))
			return EBUSY;
	} else {
		mutex_enter(&sc->sc_lock);
	}

	return 0;
}

static void mtk_i2c_release_bus(void *priv, int flags)
{
	struct mtk_i2c_softc *sc = priv;

	mutex_exit(&sc->sc_lock);
}

static int mtk_i2c_exec(void *priv, i2c_op_t op, i2c_addr_t addr,
			const void *cmdbuf, size_t cmdlen,
			void *buf, size_t len, int flags)
{
	struct mtk_i2c_softc *sc = priv;
	struct mt_i2c *i2c = &(sc->i2c);
	struct i2c_msg msgs[2];
	uint8_t *wr_buf = NULL;
	void *cmd_buf = __UNCONST(cmdbuf);
	int msgs_num;
	int ret = 0;

	if ((cmdlen == 0) && (len == 0)) {
		printf("[I2C] id[%d] cmdlen == len == 0, return directly!\n", i2c->id);
		return EINVAL;
	}

	KASSERT(mutex_owned(&sc->sc_lock));

	i2c->dma_en = false;
	i2c->auto_restart = false;
	i2c->filter_msg = false;
	i2c->speed = MAX_ST_MODE_SPEED;

	if (I2C_OP_WRITE_P(op)) {
		wr_buf = (uint8_t *)kmem_zalloc((cmdlen + len), KM_SLEEP);
		if (cmdlen > 0)
			memcpy(wr_buf, (uint8_t *)cmd_buf, cmdlen);
		memcpy((wr_buf + cmdlen), (uint8_t *)buf, len);

		msgs[0].addr = addr;
		msgs[0].flags = 0;
		msgs[0].buf = wr_buf;
		msgs[0].len = cmdlen + len;

		ret = mtk_i2c_transfer(sc, msgs, 1, flags);
		if (ret != 0)
			printf("[I2C]write fail(%d), id[%d] addr[%x] cmdlen[%zd] len[%zd]\n",
			       ret, i2c->id, addr, cmdlen, len);
	} else {
		if (cmdlen > 0)
			msgs_num = 2;
		else
			msgs_num = 1;

		msgs[0].addr = addr;
		msgs[0].flags = 0;
		msgs[0].buf = (uint8_t *)cmd_buf;
		msgs[0].len = cmdlen;

		msgs[1].addr = addr;
		msgs[1].flags = 1;
		msgs[1].buf = (uint8_t *)buf;
		msgs[1].len = len;

		ret = mtk_i2c_transfer(sc, &msgs[2 - msgs_num], msgs_num, flags);
		if (ret != 0)
			printf("[I2C]read fail(%d), id[%d] addr[%x] cmdlen[%zd] len[%zd]\n",
			       ret, i2c->id, addr, cmdlen, len);
	}

	return ret;
}

