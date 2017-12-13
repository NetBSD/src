struct eth_tndesc {
	uint32_t tn_ctl;
	uint32_t tn_sdp;
	uint32_t tn_vtag;
	uint32_t tn_lso;
};

#define	TN_CTL_COWN	__BIT(31)	// CPU Ownership
#define TN_CTL_EOR	__BIT(30)	// End Of Ring
#define TN_CTL_FS	__BIT(29)	// First Segment
#define TN_CTL_LS	__BIT(28)	// Last Segment
#define TN_CTL_INT	__BIT(27)	// Interrupt upon transmission
#define TN_CTL_INSV	__BIT(26)	// Insert VLAN Tag
#define	TN_CTL_ICO	__BIT(25)	// Enable IP checksum offload
#define	TN_CTL_UCO	__BIT(24)	// Enable UDP checksum offload
#define	TN_CTL_TCO	__BIT(23)	// Enable TCP checksum offload
#define TN_CTL_VTG	__BIT(22)	// VLAN Tag
#define	TN_CTL_ICOE	__BIT(21)	// IP checksum offload error
#define	TN_CTL_UCOE	__BIT(20)	// UDP checksum offload error
#define	TN_CTL_TCOE	__BIT(19)	// TCP checksum offload error
#define TN_CTL_LSO	__BIT(18)	// Enable LSO function
#define TN_CTL_INCID	__BIT(17)	// Increment IP ID Enable
#define TN_CTL_SDL	__BITS(15,0)	// Segment Data Length

#define TN_VTAG_EPID	__BITS(31,16)	// VLAN Tag EPID
#define TN_VTAG_PRI	__BITS(15,13)	// VLAN Tag Priority
#define TN_VTAG_CFI	__BIT(12)	// VLAN Tag CFI (Canonical Format Indidcator)
#define TN_VTAG_VID	__BITS(11,0)	// VLAN Tag VID

#define	TN_LSO_MSS	__BITS(31,21)	// TCP MSS Value used in LSO
#define TN_LSO_PKTLEN	__BITS(20,0)	// The LSO total length

struct eth_fndesc {
	uint32_t fn_sdp;
	uint32_t fn_ctl;
	uint32_t fn_vtag;
	uint32_t fn__rsvrd;
};

#define	FN_CTL_COWN	__BIT(31)	// CPU Ownership
#define FN_CTL_EOR	__BIT(30)	// End Of Ring
#define FN_CTL_FS	__BIT(29)	// First Segment
#define FN_CTL_LS	__BIT(28)	// Last Segment
#define FN_CTL__BIT26	__BIT(27)	// Not used
#define FN_CTL_IPV6	__BIT(26)	// Bit 2 of PROT
#define	FN_CTL_OSIZE	__BIT(25)	// Oversized packet
#define	FN_CTL_CRCE	__BIT(24)	// CRC error
#define	FN_CTL_RMC	__BIT(23)	// Destination MAC is reserved multicast
#define FN_CTL_HHIT	__BIT(22)	// Destination MAC is hit in hash table
#define	FN_CTL_MYMAC	__BIT(21)	// Destination MAC is My MAC
#define	FN_CTL_VTED	__BIT(20)	// VTAG Tag present
#define	FN_CTL_PROT01	__BITS(19,18)	// Bits 0:1 of PROT
#define FN_CTL_IPF	__BIT(17)	// IP Checksum failed
#define FN_CTL_L4F	__BIT(16)	// Layer 4 checksum Failed
#define FN_CTL_SDL	__BITS(15,0)	// Segment Data Length

#define FN_VTAG_EPID	__BITS(31,16)	// VLAN Tag EPID
#define FN_VTAG_PRI	__BITS(15,13)	// VLAN Tag Priority
#define FN_VTAG_CFI	__BIT(12)	// VLAN Tag CFI (Canonical Format Indidcator)
#define FN_VTAG_VID	__BITS(11,0)	// VLAN Tag VID

#define PROT_UNKNOWN	7
#define PROT_TCPV6	6	// TCP checksum
#define PROT_UDPV6	5	// UDP checksum
#define PROT_IPV6	4	// (IPV6 Fragment) || (IPV6 && !TCP && !UDP)
#define PROT_TCPV4	2	// TCP & IP checksums
#define PROT_UDPV4	1	// UDP & IP checksums
#define PROT_IPV4	0	// (IPV4 Fragment) || (IPV4 && !TCP && !UDP)

#define PHY_CTRL0	0x0000	// 
#define  PHY_CTRL0_RW_DATA	__BITS(31,16)
#define  PHY_CTRL0_RW_OK	__BIT(15)
#define  PHY_CTRL0_CL22_RD_CMD	__BIT(14)
#define  PHY_CTRL0_CL22_WR_CMD	__BIT(13)
#define  PHY_CTRL0_PHY_REG	__BITS(12,8)
#define  PHY_CTRL0_CL45_CMD_SEL	__BITS(7,6)
#define  PHY_CTRL0_CL45_CMD	__BIT(5)
#define  PHY_CTRL0_PHY_ADDR	__BITS(4,0)
#define PHY_CTRL1	0x0004	// 
#define  PHY_CTRL1_PHY_ADDR_AUTO __BITS(28,24)
#define  PHY_CTRL1_SWH_CK_PD	__BIT(23)
#define  PHY_CTRL1_PHY_PORT_SEL	__BIT(22)
#define  PHY_CTRL1_EXT_MAC_SEL	__BIT(21)
#define  PHY_CTRL1_INT_PHY_PD	__BIT(19)
#define  PHY_CTRL1_RGMII_PHY	__BIT(17)
#define  PHY_CTRL1_REV_MII	__BIT(16)
#define  PHY_CTRL1_TXC_CHK_EN	__BIT(14)
#define  PHY_CTRL1_FORCE_FC_TX	__BIT(13)
#define  PHY_CTRL1_FORCE_FC_RX	__BIT(12)
#define  PHY_CTRL1_FORCE_DPX	__BIT(11)
#define  PHY_CTRL1_FORCE_SPD	__BITS(10,9)
#define  PHY_CTRL1_AN_EN	__BIT(8)
#define  PHY_CTRL1_MI_DIS	__BIT(7)
#define  PHY_CTRL1_FC_TX_ST	__BIT(6)
#define  PHY_CTRL1_FC_RX_ST	__BIT(5)
#define  PHY_CTRL1_FDPX_ST	__BIT(4)
#define  PHY_CTRL1_SPD_ST	__BITS(3,2)
#define   PHY_CTRL1_SPD_ST_10	0
#define   PHY_CTRL1_SPD_ST_100	1
#define   PHY_CTRL1_SPD_ST_1000	2
#define  PHY_CTRL1_TXC_ST	__BIT(1)
#define  PHY_CTRL1_LINK_ST	__BIT(0)
#define MAC_CFG		0x0008	// 
#define  MAC_CFG_NIC_PD		__BIT(31)
#define  MAC_CFG_WOL_PD		__BIT(30)
#define  MAC_CFG_NIC_PD_RDY	__BIT(29)
#define  MAC_CFG_RXDV_WAKEUP_EN	__BIT(28)
#define  MAC_CFG_TXPART_WAKEUP_EN	__BIT(27)
#define  MAC_CFG_TX_CKS_EN		__BIT(26)
#define  MAC_CFG_RX_CKS_EN		__BIT(25)
#define  MAC_CFG_ACPT_CKS_ERR		__BIT(24)
#define  MAC_CFG_INS_EN		__BIT(23)
#define  MAC_CFG_VLAN_STRIP		__BIT(22)
#define  MAC_CFG_ACPT_CRC_ERR		__BIT(21)
#define  MAC_CFG_CRC_STRIP		__BIT(20)
#define  MAC_CFG_TX_AUTO_PAD		__BIT(19)
#define  MAC_CFG_ACPT_LONG_PKT		__BIT(18)
#define  MAC_CFG_MAC_LEN		__BITS(17,16)
#define  MAC_CFG_MAC_LEN_1518		0
#define  MAC_CFG_MAC_LEN_1522		1
#define  MAC_CFG_MAC_LEN_1536		2
#define  MAC_CFG_MAC_LEN_AUTO		3
#define  MAC_CFG_UDPV6_DROP0CKSUM	__BIT(15)
#define  MAC_CFG_IPG			__BITS(14,10)
#define  MAC_CFG_IPG_96			0x1f
#define  MAC_CFG_IPG_88			0x1b
#define  MAC_CFG_IPG_80			0x17
#define  MAC_CFG_IPG_72			0x13
#define  MAC_CFG_IPG_64			0x0f
#define  MAC_CFG_IPG_56			0x0b
#define  MAC_CFG_IPG_48			0x07
#define  MAC_CFG_IPG_40			0x06
#define  MAC_CFG_IPG_32			0x05
#define  MAC_CFG_DONOT_SKIP		__BIT(9)
#define  MAC_CFG_FAST_RETRY		__BIT(8)
#define  MAC_CFG_TX_VLANTAG_AUTO_PAR	__BIT(0)
#define FC_CFG		0x000c	// 
#define  FC_CFG_SEND_PAUSE_TH	__BITS(27,16)
#define  FC_CFG_COLCNT_CLR_MIDE	__BIT(9)
#define  FC_CFG_UC_PAUSE_DIS	__BIT(8)
#define  FC_CFG_BP_EN		__BIT(7)
#define  FC_CFG_CRS_BP_MODE	__BIT(6)
#define  FC_CFG_MAX_BP_COL_EN	__BIT(5)
#define  FC_CFG_MAX_BP_COL_CNT	__BITS(4,0)
#define ARL_CFG		0x0010	// 
#define  ARL_CFG_HASH_MULTICAST_ONLY	__BIT(7)
#define  ARL_CFG_PRI_TAG_FIL		__BIT(6)
#define  ARL_CFG_VLAN_UTAG_FIL	__BIT(5)
#define  ARL_CFG_MISC_MODE	__BIT(4)
#define  ARL_CFG_MY_MAC_ONLY	__BIT(3)
#define  ARL_CFG_CPU_LEN_DIS	__BIT(2)
#define  ARL_CFG_REV_MC_FILTER	__BIT(1)
#define  ARL_CFG_HASH_ALG	__BIT(0)
#define MYMAC0_H	0x0014	// Bits [47:32] (first DA byte is mSB)
#define MKMYMAC_H(da0,da1) \
	(((da0) << 8)|(da1))
#define MYMAC0_L	0x0018	// Bits [31:0]
#define MKMYMAC_L(da2,da3,da4,da5) \
	(((da2) << 24)|((da3) << 16)|((da4) << 8)|(da5))
#define HASH_CTRL	0x001c	// 
#define  HASH_CTRL_HT_BIST_EN		__BIT(31)
#define  HASH_CTRL_HT_BIST_DONE		__BIT(17)
#define  HASH_CTRL_HT_BIST_OK		__BIT(16)
#define  HASH_CTRL_HASH_ACC_WR_CMD	__BIT(13)
#define  HASH_CTRL_HASH_BIT_DATA	__BIT(12)
#define  HASH_CTRL_HASH_BIT_ADDRESS	__BITS(8,0)
#define VLAN_CTRL	0x0020	// 
#define  VLAN_CTL_MY_VID3_EN		__BIT(3)
#define  VLAN_CTL_MY_VID2_EN		__BIT(2)
#define  VLAN_CTL_MY_VID1_EN		__BIT(1)
#define  VLAN_CTL_MY_VID0_EN		__BIT(0)
#define VLAN_ID_01	0x0024	// 
#define  VLAN_ID_MYVID1		__BITS(27,16)
#define  VLAN_ID_MYVID0		__BITS(11,0)
#define VLAN_ID_23	0x0028	// 
#define  VLAN_ID_MYVID2		__BITS(27,16)
#define  VLAN_ID_MYVID3		__BITS(11,0)
#define DUMMY_CTRL	0x002c	// 
#define  DUMMY_CTRL_TXC_EXIST_EN	__BIT(4)
#define  DUMMY_CTRL_WR_CLR_MIB		__BIT(3)
#define  DUMMY_CTRL_NO_COL_PIN		__BIT(2)
#define  DUMMY_CTRL_TX_RX_PD_RDY	__BIT(1)
#define  DUMMY_CTRL_MDIO_CMD_DONE	__BIT(0)
#define DMA_CFG		0x0030	// 
#define  DMA_CFG_RX_OFFSET_2B_DIS	__BIT(16)
#define  DMA_CFG_TX_CKSERR_WRKB_DIS	__BIT(8)
#define  DMA_CFG_TX_POLL_PERIOD		__BIT(7,6)
#define  DMA_CFG_XX_POLL_PERIOD_1US	0
#define  DMA_CFG_XX_POLL_PERIOD_10US	1
#define  DMA_CFG_XX_POLL_PERIOD_100US	2
#define  DMA_CFG_XX_POLL_PERIOD_1000US	3
#define  DMA_CFG_TX_POLL_END		__BIT(5)
#define  DMA_CFG_TX_SUSPEND		__BIT(4)
#define  DMA_CFG_RX_POLL_PERIOD		__BITS(3,2)
#define  DMA_CFG_RX_POLL_EN		__BIT(1)
#define  DMA_CFG_RX_SUSPEND		__BIT(0)
#define TX_DMA_CTRL	0x0034	// 
#define  TX_DMA_CTRL_TX_EN		__BIT(3)
#define  TX_DMA_CTRL_TX_RESUME		__BIT(2)
#define  TX_DMA_CTRL_TX_STOP		__BIT(1)
#define  TX_DMA_CTRL_TX_START		__BIT(0)
#define RX_DMA_CTRL	0x0038	// 
#define  RX_DMA_CTRL_TX_EN		__BIT(3)
#define  RX_DMA_CTRL_TX_RESUME		__BIT(2)
#define  RX_DMA_CTRL_TX_STOP		__BIT(1)
#define  RX_DMA_CTRL_TX_START		__BIT(0)
#define TX_DPTR		0x003c	// 16-byte aligned
#define RX_DPTR		0x0040	// 16-byte aligned
#define TX_BASE_ADDR	0x0044	//
#define RX_BASE_ADDR	0x0048	//
		//	0x004c
#define INT_STS		0x0050	// 
#define  INT_ARP			__BIT(24)
#define  INT_WOL			__BIT(23)
#define  INT_LSO_FIFO_FULL		__BIT(22)
#define  INT_LSO_FIFO_EMPTY		__BIT(21)
#define  INT_LSO_LEN_ERR		__BIT(20)
#define  INT_LSO_PRO_ERR		__BIT(19)
#define  INT_TIMER			__BIT(18)
#define  INT_RX_AXI_ERR			__BIT(17)
#define  INT_TX_AXI_ERR			__BIT(16)
#define  INT_IP_CKS			__BIT(15)
#define  INT_TCP_CKS			__BIT(14)
#define  INT_UDP_CKS			__BIT(13)
#define  INT_PHY			__BIT(12)
#define  INT_RX_RCF			__BIT(11)
#define  INT_RX_PCODE			__BIT(10)
#define  INT_TX_SKIP			__BIT(9)
#define  INT_TNTC			__BIT(8)
#define  INT_TNQE			__BIT(7)
#define  INT_FNRC			__BIT(6)
#define  INT_FNQF			__BIT(5)
#define  INT_RX_MAGIC_PKT		__BIT(4)
#define  INT_MIB_CNT_TH			__BIT(3)
#define  INT_PORT_STS_CHG		__BIT(2)
#define  INT_RX_FIFO_FULL		__BIT(1)
#define INT_MASK	0x0054	// 
#define TEST0		0x0058	// 
#define TEST1		0x005c	// 
#define EXT_CFG		0x0060	// 
#define  EXT_CFG_ALLOW_DOUBLE_PAUSE	__BIT(31)
#define  EXT_CFG_SEND_PAUSE_RLS		__BITS(26,16)
		//	0x0064
		//	0x0068
#define ETHPHY_CFG	0x006c	// 
#define  ETHPHY_CFG_ADC_IN_MUX_EN	__BIT(8)
#define  ETHPHY_CFG_AFE_TEST_MODE	__BIT(5)
#define  ETHPHY_CFG_AFE_TEST_OE		__BIT(4)
#define  ETHPHY_CFG_RFC_CMI_ENB		__BIT(2)
#define  ETHPHY_CFG_PIF_CLK_BYP		__BIT(1)
#define  ETHPHY_CFG_PLL_CLK_BYP		__BIT(0)
#define DMASTS_CFG	0x0070	// 
#define  DMASTS_CFG_TN_C_BIT		__BIT(16)
#define  DMASTS_CFG_TN_E_BIT		__BIT(15)
#define  DMASTS_CFG_TN_L_BIT		__BIT(14)
#define  DMASTS_CFG_TN_I_BIT		__BIT(13)
#define  DMASTS_CFG_TN_LSE_BIT		__BIT(12)
#define  DMASTS_CFG_TN_DMA_STS		__BITS(11,7)
#define  DMASTS_CFG_FN_C_BIT		__BIT(6)
#define  DMASTS_CFG_FN_E_BIT		__BIT(5)
#define  DMASTS_CFG_FN_DMA_STS		__BITS(4,0)
#define DMABURST_CFG	0x0074	// 
#define  DMABURST_CFG_AXI_BURST_LEN	__BITS(17,16)
#define  DMABURST_CFG_AXI_BURST_LEN_128	0
#define  DMABURST_CFG_AXI_BURST_LEN_64	1
#define  DMABURST_CFG_AXI_BURST_LEN_32	2
#define  DMABURST_CFG_AXI_BURST_LEN_032	3
#define  DMABURST_CFG_TX_AXI_BURST	__BITS(14,12)
#define  DMABURST_CFG_XX_AXI_BURST_128	0
#define  DMABURST_CFG_XX_AXI_BURST_256	1
#define  DMABURST_CFG_XX_AXI_BURST_512	2
#define  DMABURST_CFG_XX_AXI_BURST_1024	3
#define  DMABURST_CFG_XX_AXI_BURST_2048	4
#define  DMABURST_CFG_XX_AXI_BURST_4096	6
#define  DMABURST_CFG_RX_AXI_BURST	__BITS(10,8)
#define  DMABURST_CFG_TX_BURST		__BITS(6,4)
#define  DMABURST_CFG_XX_BURST_INCR4	3
#define  DMABURST_CFG_XX_BURST_INCR8	5
#define  DMABURST_CFG_XX_BURST_INCR16	7
#define  DMABURST_CFG_RX_BURST		__BITS(2,0)
#define EEE_CTRL	0x0078	// 
#define  EEE_CTRL_RFC_LP1		__BIT(1)
#define  EEE_CTRL_LP1_MODE_EN		__BIT(0)
#define EEE_SLP_TIME	0x007c	// 
#define  EEE_SLP_TIME_LP1_TH		__BITS(26,0) // 40ns Units
#define EEE_WAKEUP	0x0080	// 
#define  EEE_WAKEUP_LP1_TH		__BITS(26,0) // 40ns Units
#define PHY_CONF1	0x0084	// 
#define  PHY_CONF1_PLL_MODE_EN		__BIT(5)
#define  PHY_CONF1_CHIP_SMI_ADDR	__BITS(4,0)
#define PHY_CONF2	0x0088	// 
#define  PHY_CONF2_PART_NUM		__BITS(21,16)
#define  PHY_CONF2_REV_CODE		__BITS(9,0)
#define PHY_MON		0x008c	// 
#define PHY_CLK_CTRL	0x0090	// 
#define SYS_CONF	0x0094	// 
#define  SYS_CONF_INT_PHY_SEL		__BIT(3)
#define  SYS_CONF_SWC_MII_MODE		__BIT(2)
#define  SYS_CONF_EXT_MDC_MODE		__BIT(1)
#define  SYS_CONF_MII_PAD_OE		__BIT(0)
#define MAC_CONF	0x0098	// 
#define  MAC_CONF_RMII_EXT_SRC		__BIT(1)
#define  MAC_CONF_BIG_ENDIAN		__BIT(0)
#define SW_RESET	0x009c	// 
#define  SW_RESET_TX_RST_B		__BIT(6)
#define  SW_RESET_RX_RST_B		__BIT(5)
#define  SW_RESET_PHY_RSTN		__BIT(4)
#define  SW_RESET_M_RSTN		__BIT(3)
#define  SW_RESET_N_RSTN		__BIT(2)
#define  SW_RESET_H_RSTN		__BIT(1)
#define  SW_RESET_D_RSTN		__BIT(0)
#define MAC_DBG1	0x00a0	// 
#define MAC_DBG2	0x00a4	// 
#define MAC_DBG3	0x00a8	// 
#define MAC_CLK_CONF	0x00ac	// 
#define  MAC_CLK_CONF_GTXC_OUT_INV	__BIT(21)
#define  MAC_CLK_CONF_RMII_OUT_INV	__BIT(20)
#define  MAC_CLK_CONF_TXCLK_OUT_INV	__BIT(19)
#define  MAC_CLK_CONF_RXCLK_OUT_INV	__BIT(18)
#define  MAC_CLK_CONF_TXCLK_IN_INV	__BIT(17)
#define  MAC_CLK_CONF_RXCLK_IN_INV	__BIT(16)
#define  MAC_CLK_CONF_SW_MAC_DIV_EN	__BIT(14)
#define  MAC_CLK_CONF_SW_MAC_DIV	__BITS(13,12)
#define  MAC_CLK_CONF_SW_MAC_DIV_2500	0
#define  MAC_CLK_CONF_SW_MAC_DIV_25000	1
#define  MAC_CLK_CONF_SW_MAC_DIV_MAC	2
#define  MAC_CLK_CONF_MDC_INV		__BIT(9)
#define  MAC_CLK_CONF_MDIO_NEG_LAT	__BIT(8)
#define  MAC_CLK_CONF_MDC_DIV		__BITS(7,0)
#define PHY_ADC_CALI0	0x00b0	// 
#define  PHY_ADC_CALI0_CAL_BYP		__BIT(31)
#define  PHY_ADC_CALI0_CAL_WAIT_TIMER	__BITS(22,16)
#define  PHY_ADC_CALI0_OFFSET_ACC_CNT	__BITS(9,0)
#define PHY_ADC_CALI1	0x00b4	// 
#define  PHY_ADC_CALI1_ANA_PDEN		__BIT(31)
#define  PHY_ADC_CALI1_ANA_PDCTL	__BITS(20,0)
#define MAC_DBG4	0x00b8	// 

//	Counters

#define C_RX_ARP	0x00fc	// 
#define C_RX_OKPKT	0x0100	// 
#define C_RX_OKBYTE	0x0104	// 
#define C_RX_RUNT	0x0108	// 
#define C_RX_LONG	0x010c	// 
#define C_RX_DROP	0x0110	// 
#define C_RX_CRC	0x0114	// 
#define C_RX_ARLDROP	0x0118	// 
#define C_RX_VLANDROP	0x011c	// 
#define C_RX_CSERR	0x0120	// 
#define C_RX_PAUSE	0x0124	// 
#define C_TX_OKPKT	0x0128	// 
#define C_TX_OKBYTE	0x012c	// 
#define C_TX_PAULCOL	0x0130	// 
#define C_TX_RTY	0x0134	// 
#define C_TX_SKIP	0x0138	// 
#define C_TX_ARP	0x013c	// 

//	Mac

#define MYMACX_EN	__BIT(31)
#define	MYMAC1_H	0x0140	// 
#define	MYMAC1_L	0x0144	// 
#define	MYMAC2_H	0x0148	// 
#define	MYMAC2_L	0x014c	// 
#define	MYMAC3_H	0x0150	// 
#define	MYMAC3_L	0x0154	// 
#define	MYMAC4_H	0x0158	// 
#define	MYMAC4_L	0x015c	// 
#define	MYMAC5_H	0x0160	// 
#define	MYMAC5_L	0x0164	// 
#define	MYMAC6_H	0x0168	// 
#define	MYMAC6_L	0x016c	// 
#define	MYMAC7_H	0x0170	// 
#define	MYMAC7_L	0x0174	// 
#define	MYMAC8_H	0x0178	// 
#define	MYMAC8_L	0x017c	// 
#define	MYMAC9_H	0x0180	// 
#define	MYMAC9_L	0x0184	// 
#define	MYMAC10_H	0x0188	// 
#define	MYMAC10_L	0x018c	// 
#define	MYMAC11_H	0x0190	// 
#define	MYMAC11_L	0x0194	// 
#define	MYMAC12_H	0x0198	// 
#define	MYMAC12_L	0x019c	// 
#define	MYMAC13_H	0x01a0	// 
#define	MYMAC13_L	0x01a4	// 
#define	MYMAC14_H	0x01a8	// 
#define	MYMAC14_L	0x01ac	// 
#define	MYMAC15_H	0x01b0	// 
#define	MYMAC15_L	0x01b4	// 
#define	C_MISC		0x01b8	// 
#define	RCF_SRAM_CFG0	0x01bc	// 
#define	RCF_SRAM_CFG1	0x01c0	// 
#define	RCF_SRAM_CFG2	0x01c4	// 
#define	RCF_SRAM_CFG3	0x01c8	// 
#define	RCF_CFG_R0	0x01cc	// 
#define	RCF_CFG_WRDY	0x01d0	// 
#define	HASH_CFG	0x01d4	// 
#define	C_RX_ERR	0x01d8	// 
#define	C_RX_UNI	0x01dc	// 
#define	C_RX_MULTI	0x01e0	// 
#define	C_RX_BROAD	0x01e4	// 
#define	C_RX_ALIGNERR	0x01e8	// 
#define	C_TX_UNI	0x01ec	// 
#define	C_TX_MULTI	0x01f0	// 
#define	C_TX_BROAD	0x01f4	// 
#define	C_TX_ERR	0x01f8	// 
#define	C_TX_UNI	0x01fc	// 
#define	PHYINT_CFG	0x0200	// 
#define	INT_TIMER	0x0204	// 
#define	LENGTH_ERR	0x0208	// 
#define	AXI_ERR_LATCH	0x020c	// 
#define	LSO_CFG		0x0210	// 
#define	C_RX_LENGTHERR	0x0214	// 
#define	C_RX_TWIST	0x0218	// 
#define	WOL_MRXBUF_CTL	0x021c	// 
#define	C_STBY_CTRL	0x0220	// 
#define	WOL_SRAM_CFG1	0x0224	// 
#define	WOL_SRAM_CFG2	0x0228	// 
#define	WOL_SRAM_CFG3	0x022c	// 
#define	WOL_SRAM_CFG4	0x0230	// 
#define	WOL_SRAM_CFG5	0x0234	// 
#define	WOL_SRAM_CFG6	0x0238	// 
#define	WOL_SRAM_CFG7	0x023c	// 
#define	WOL_SRAM_CFG8	0x0240	// 
#define	ARP_CFG0	0x0244	// 
#define	ARP_CFG1	0x0248	// 
#define	ARP_CFG2	0x024c	// 
#define	ARP_CFG3	0x0250	// 
#define	ARP_CFG4	0x0254	// 
#define	ARP_CFG5	0x0258	// 
#define	WOL_SRAM_STA	0x025c	// 
#define	MGP_LENGTH_STA	0x0260	// 
#define MGP_STAn(n0	(0x264 + 4*(n)	// 
#define WOL_DET_LA	0x2e4	//
#define WOL_CHKLEN0	0x2e8	//
#define WOL_CHKLEN1	0x2ec	//
#define WOL_CHKLEN2	0x2f0	//
#define WOL_CHKLEN3	0x2f8	//
#define WOL_CHKLEN4	0x2fc	//
#define WOL_PAT_EN	0x2fc	//

