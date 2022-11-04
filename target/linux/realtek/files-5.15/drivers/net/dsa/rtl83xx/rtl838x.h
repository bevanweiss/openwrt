/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _RTL838X_H
#define _RTL838X_H __FILE__

#include <linux/bitops.h>
#include <linux/types.h>
#include <net/dsa.h>

#include "rtl83xx.h"

#define RTL8380_VERSION_A 'A'
#define RTL8380_VERSION_B 'B'

/* Register definitions */
#define RTL838X_MAC_FORCE_MODE_CTRL_REG(p)              (0xa104 + ((p) * 0x4))
/* Reserved                                                     31 - 28 */
#define RTL838X_MAC_FORCE_MODE_CTRL_PHY_POWER_SEL               BIT(27)
#define RTL838X_MAC_FORCE_MODE_CTRL_RETRY_SPDN_GLITE            BIT(26)
#define RTL838X_MAC_FORCE_MODE_CTRL_EN_MY_SOFTNP                BIT(25)
#define RTL838X_MAC_FORCE_MODE_CTRL_RETRY_SPDN_10M              BIT(24)
#define RTL838X_MAC_FORCE_MODE_CTRL_SPDN_THR                    BIT(23)
#define RTL838X_MAC_FORCE_MODE_CTRL_EN_RETRY_SPD_DN             BIT(22)
#define RTL838X_MAC_FORCE_MODE_CTRL_EN_2PAIR_SPD_DN             BIT(21)
#define RTL838X_MAC_FORCE_MODE_CTRL_BYPASS_GLITE_UP1            BIT(20)
#define RTL838X_MAC_FORCE_MODE_CTRL_GLITE_MASTER_SLV_MANUAL_EN  BIT(19)
#define RTL838X_MAC_FORCE_MODE_CTRL_GLITE_MASTER_SLV_MANUAL_SEL BIT(18)
#define RTL838X_MAC_FORCE_MODE_CTRL_GLITE_PORT_TYPE             BIT(17)
#define RTL838X_MAC_FORCE_MODE_CTRL_GLITE_EEE                   BIT(16)
#define RTL838X_MAC_FORCE_MODE_CTRL_GLITE_SEL                   BIT(15)
#define RTL838X_MAC_FORCE_MODE_CTRL_MEDIA_SEL                   BIT(14)
#define RTL838X_MAC_FORCE_MODE_CTRL_PHY_MASTER_SLV_MANUAL_EN    BIT(13)
#define RTL838X_MAC_FORCE_MODE_CTRL_PHY_MASTER_SLV_MANUAL_SEL   BIT(12)
#define RTL838X_MAC_FORCE_MODE_CTRL_PHY_PORT_TYPE               BIT(11)
#define RTL838X_MAC_FORCE_MODE_CTRL_EEE_1000M_EN                BIT(10)
#define RTL838X_MAC_FORCE_MODE_CTRL_EEE_100M_EN                 BIT(9)
#define RTL838X_MAC_FORCE_MODE_CTRL_FC_EN                       BIT(8)
#define RTL838X_MAC_FORCE_MODE_CTRL_RX_PAUSE_EN                 BIT(7)
#define RTL838X_MAC_FORCE_MODE_CTRL_TX_PAUSE_EN                 BIT(6)
#define RTL838X_MAC_FORCE_MODE_CTRL_SPD_SEL                     GENMASK(5, 4)
#define RTL838X_MAC_FORCE_MODE_CTRL_SPD_SEL_1000M                       0b10
#define RTL838X_MAC_FORCE_MODE_CTRL_SPD_SEL_100M                        0b01
#define RTL838X_MAC_FORCE_MODE_CTRL_SPD_SEL_10M                         0b00
#define RTL838X_MAC_FORCE_MODE_CTRL_DUP_SEL                     BIT(3)
#define RTL838X_MAC_FORCE_MODE_CTRL_NWAY_EN                     BIT(2)
#define RTL838X_MAC_FORCE_MODE_CTRL_LINK_EN                     BIT(1)
#define RTL838X_MAC_FORCE_MODE_CTRL_EN                          BIT(0)

#define RTL838X_MAC_PORT_CTRL_REG(p)                    (0xd560 + ((p) * 0x80))
/* Reserved                                                     31 - 15 */
#define RTL838X_MAC_PORT_CTRL_RX_FIFO_ERROR                     BIT(14)
#define RTL838X_MAC_PORT_CTRL_RX_ENTRY_ERROR                    BIT(13)
#define RTL838X_MAC_PORT_CTRL_TX_FIFO_ERROR                     BIT(12)
#define RTL838X_MAC_PORT_CTRL_TX_ENTRY_ERROR                    BIT(11)
#define RTL838X_MAC_PORT_CTRL_RX_RXER_CHK_EN                    BIT(10)
#define RTL838X_MAC_PORT_CTRL_BYP_TX_CRC                        BIT(9)
#define RTL838X_MAC_PORT_CTRL_PASS_ALL_MODE_EN                  BIT(8)
#define RTL838X_MAC_PORT_CTRL_PRECOLLAT_SEL                     GENMASK(7, 6)
#define RTL838X_MAC_PORT_CTRL_LATE_COLI_THR                     GENMASK(5, 4)
#define RTL838X_MAC_PORT_CTRL_RX_CHK_CRC_EN                     BIT(3)
#define RTL838X_MAC_PORT_CTRL_BKPRES_EN                         BIT(2)
#define RTL838X_MAC_PORT_CTRL_TX_EN                             BIT(1)
#define RTL838X_MAC_PORT_CTRL_RX_EN                             BIT(0)
#define RTL838X_MAC_PORT_CTRL_TXRX_EN \
        (RTL838X_MAC_PORT_CTRL_TX_EN | RTL838X_MAC_PORT_CTRL_RX_EN)

#define RTL838X_MAC_FORCE_MODE_CTRL		(0xa104)
#define RTL838X_MAC_PORT_CTRL(port)		(0xd560 + (((port) << 7)))
#define RTL838X_PORT_ISO_CTRL(port)		(0x4100 + ((port) << 2))

/* Packet statistics */
#define RTL838X_STAT_CTRL			(0x3108)
#define RTL838X_STAT_PORT_RST			(0x3104)
#define RTL838X_STAT_PORT_STD_MIB		(0x1200)
#define RTL838X_STAT_RST			(0x3100)

/* Registers of the internal Serdes */
#define RTL838X_SDS4_DUMMY0			(0xef8c)
#define RTL838X_SDS4_FIB_REG0			(0xf800)
#define RTL838X_SDS4_REG28			(0xef80)
#define RTL838X_SDS5_EXT_REG6			(0xf18c)
#define RTL838X_SDS_CFG_REG			(0x0034)
#define RTL838X_SDS_MODE_SEL			(0x0028)

/* VLAN registers */
#define RTL838X_VLAN_CTRL			(0x3a74)
#define RTL838X_VLAN_FID_CTRL			(0x3aa8)
#define RTL838X_VLAN_PORT_EGR_FLTR		(0x3a84)
#define RTL838X_VLAN_PORT_FWD			(0x3a78)
#define RTL838X_VLAN_PORT_IGR_FLTR		(0x3a7c)
#define RTL838X_VLAN_PORT_PB_VLAN		(0x3c00)
#define RTL838X_VLAN_PROFILE(idx)		(0x3a88 + ((idx) << 2))

/* Table access registers */
#define RTL838X_TBL_ACCESS_CTRL_0		(0x6914)
#define RTL838X_TBL_ACCESS_DATA_0(idx)		(0x6918 + ((idx) << 2))
#define RTL838X_TBL_ACCESS_CTRL_1		(0xa4c8)
#define RTL838X_TBL_ACCESS_DATA_1(idx)		(0xa4cc + ((idx) << 2))

/* MAC handling */
#define RTL838X_MAC_LINK_DUP_STS_REG(p)                 (0xa19c + (((p) / 32) * 0x4))
#define _RTL838X_MAC_LINK_DUP_STS_MASK                          BIT(0)
#define RTL838X_MAC_LINK_DUP_STS_FULL                                   1
#define RTL838X_MAC_LINK_DUP_STS_HALF                                   0
#define RTL838X_MAC_LINK_DUP_STS(p, r) \
        (((r) >> ((p) % 32)) & _RTL838X_MAC_LINK_DUP_STS_MASK)

#define RTL838X_MAC_LINK_SPD_STS_REG(p)                 (0xa190 + (((p) / 16) * 0x4))
#define _RTL838X_MAC_LINK_SPD_STS_MASK                          GENMASK(1, 0)
#define RTL838X_MAC_LINK_SPD_STS_2G                                     0x3 /* Only for port 24 & 26 */
#define RTL838X_MAC_LINK_SPD_STS_1000M                                  0x2
#define RTL838X_MAC_LINK_SPD_STS_100M                                   0x1
#define RTL838X_MAC_LINK_SPD_STS_10M                                    0x0
#define RTL838X_MAC_LINK_SPD_STS(p, r) \
        (((r) >> (((p) % 16) * 2)) & _RTL838X_MAC_LINK_SPD_STS_MASK)

#define RTL838X_MAC_LINK_STS_REG(p)                     (0xa188 + (((p) / 32) * 0x4))
#define _RTL838X_MAC_LINK_STS_MASK                              BIT(0)
#define RTL838X_MAC_LINK_STS_UP                                         0b1
#define RTL838X_MAC_LINK_STS_DOWN                                       0b0
#define RTL838X_MAC_LINK_STS(p, r) \
        (((r) >> ((p) % 32)) & _RTL838X_MAC_LINK_STS_MASK)

#define RTL838X_MAC_RX_PAUSE_STS_REG(p)                 (0xa1a4 + (((p) / 32) * 0x4))
#define _RTL838X_MAC_RX_PAUSE_STS_MASK                          BIT(0)
#define RTL838X_MAC_RX_PAUSE_STS_ON                                     0b1
#define RTL838X_MAC_RX_PAUSE_STS_OFF                                    0b0
#define RTL838X_MAC_RX_PAUSE_STS(p, r) \
        (((r) >> ((p) % 32)) & _RTL838X_MAC_RX_PAUSE_STS_MASK)

#define RTL838X_MAC_TX_PAUSE_STS_REG(p)                 (0xa1a0 + (((p) / 32) * 0x4))
#define _RTL838X_MAC_TX_PAUSE_STS_MASK                          BIT(0)
#define RTL838X_MAC_TX_PAUSE_STS_ON                                     0b1
#define RTL838X_MAC_TX_PAUSE_STS_OFF                                    0b0
#define RTL838X_MAC_TX_PAUSE_STS(p, r) \
        (((r) >> ((p) % 32)) & _RTL838X_MAC_TX_PAUSE_STS_MASK)

#define RTL838X_MAC_LINK_DUP_STS_ADDR		(0xa19c)
#define RTL838X_MAC_LINK_SPD_STS_PORT_ADDR(p)	(0xa190 + (((p >> 4) << 2)))
#define RTL838X_MAC_LINK_STS_ADDR		(0xa188)
#define RTL838X_MAC_RX_PAUSE_STS_ADDR		(0xa1a4)
#define RTL838X_MAC_TX_PAUSE_STS_ADDR		(0xa1a0)

#define RTL838X_FORCE_EN			(1 << 0)
#define RTL838X_FORCE_LINK_EN			(1 << 1)
#define RTL838X_NWAY_EN				(1 << 2)
#define RTL838X_DUPLEX_MODE			(1 << 3)
#define RTL838X_TX_PAUSE_EN			(1 << 6)
#define RTL838X_RX_PAUSE_EN			(1 << 7)
#define RTL838X_MAC_FORCE_FC_EN			(1 << 8)

/* EEE */
#define RTL838X_EEE_CLK_STOP_CTRL		(0x0148)
#define RTL838X_EEE_PORT_RX_EN			(0x0150)
#define RTL838X_EEE_PORT_TX_EN			(0x014c)
#define RTL838X_EEE_TX_TIMER_GELITE_CTRL	(0xaa08)
#define RTL838X_EEE_TX_TIMER_GIGA_CTRL		(0xaa04)
#define RTL838X_MAC_EEE_ABLTY			(0xa1a8)

/* L2 functionality */
#define RTL838X_L2_CTRL_0			(0x3200)
#define RTL838X_L2_CTRL_1			(0x3204)
#define RTL838X_L2_FLD_PMSK			(0x3288)
#define RTL838X_L2_LRN_CONSTRT			(0x329c)
#define RTL838X_L2_LRN_CONSTRT_EN		(0x3368)
#define RTL838X_L2_PORT_AGING_OUT		(0x3358)
#define RTL838X_L2_PORT_LM_ACT(p)		(0x3208 + ((p) << 2))
#define RTL838X_L2_PORT_LRN_CONSTRT		(0x32a0)
#define RTL838X_L2_PORT_NEW_SALRN(p)		(0x328c + (((p >> 4) << 2)))
#define RTL838X_L2_PORT_NEW_SA_FWD(p)		(0x3294 + (((p >> 4) << 2)))
#define RTL838X_L2_TBL_FLUSH_CTRL		(0x3370)
#define RTL838X_TBL_ACCESS_L2_CTRL		(0x6900)
#define RTL838X_TBL_ACCESS_L2_DATA(idx)		(0x6908 + ((idx) << 2))

/* Port Mirroring */
#define RTL838X_MIR_CTRL			(0x5d00)
#define RTL838X_MIR_DPM_CTRL			(0x5d20)
#define RTL838X_MIR_SPM_CTRL			(0x5d10)

/* Storm/rate control and scheduling */
#define RTL838X_SCHED_CTRL			(0xb980)
#define RTL838X_SCHED_LB_THR			(0xb984)
#define RTL838X_SCHED_LB_TICK_TKN_CTRL_0	(0xad58)
#define RTL838X_SCHED_LB_TICK_TKN_CTRL_1	(0xad5c)
#define RTL838X_SCHED_P_EGR_RATE_CTRL(p)	(0xc008 + (((p) << 7)))
#define RTL838X_SCHED_Q_EGR_RATE_CTRL(p, q)	(0xc00c + (p << 7) + (((q) << 2)))
#define RTL838X_STORM_CTRL			(0x4700)
#define RTL838X_STORM_CTRL_BURST_0		(0x487c)
#define RTL838X_STORM_CTRL_BURST_1		(0x4880)
#define RTL838X_STORM_CTRL_BURST_PPS_0		(0x4874)
#define RTL838X_STORM_CTRL_BURST_PPS_1		(0x4878)
#define RTL838X_STORM_CTRL_LB_CTRL(p)		(0x4884 + (((p) << 2)))
#define RTL838X_STORM_CTRL_PORT_BC(p)		(0x4800 + (((p) << 2)))
#define RTL838X_STORM_CTRL_PORT_BC_EXCEED	(0x470c)
#define RTL838X_STORM_CTRL_PORT_MC(p)		(0x478c + (((p) << 2)))
#define RTL838X_STORM_CTRL_PORT_MC_EXCEED	(0x4710)
#define RTL838X_STORM_CTRL_PORT_UC(p)		(0x4718 + (((p) << 2)))
#define RTL838X_STORM_CTRL_PORT_UC_EXCEED	(0x4714)

/* Link aggregation (Trunking) */
#define RTL838X_TRK_MBR_CTR			(0x3e00)
#define RTL838X_TRK_HASH_IDX_CTRL		(0x3e20)
#define RTL838X_TRK_HASH_CTRL			(0x3e24)

/* Attack prevention */
#define RTL838X_ATK_PRVNT_ACT			(0x5b08)
#define RTL838X_ATK_PRVNT_CTRL			(0x5b04)
#define RTL838X_ATK_PRVNT_PORT_EN		(0x5b00)
#define RTL838X_ATK_PRVNT_STS			(0x5b1c)

/* 802.1X */
#define RTL838X_RMA_BPDU_CTRL			(0x4330)
#define RTL838X_RMA_BPDU_FLD_PMSK		(0x4348)
#define RTL838X_RMA_LLTP_CTRL			(0x4340)
#define RTL838X_RMA_PTP_CTRL			(0x4338)
#define RTL838X_SPCL_TRAP_ARP_CTRL		(0x698c)
#define RTL838X_SPCL_TRAP_CTRL			(0x6980)
#define RTL838X_SPCL_TRAP_EAPOL_CTRL		(0x6988)
#define RTL838X_SPCL_TRAP_IGMP_CTRL		(0x6984)
#define RTL838X_SPCL_TRAP_IPV6_CTRL		(0x6994)
#define RTL838X_SPCL_TRAP_SWITCH_MAC_CTRL	(0x6998)

/* QoS */
#define RTL838X_FC_P_EGR_DROP_CTRL(p)		(0x6b1c + (((p) << 2)))
#define RTL838X_PRI_DSCP_INVLD_CTRL0		(0x5fe8)
#define RTL838X_PRI_SEL_CTRL			(0x10e0)
#define RTL838X_PRI_SEL_IPRI_REMAP		(0x5f8c)
#define RTL838X_PRI_SEL_PORT_PRI(p)		(0x5fb8 + (((p / 10) << 2)))
#define RTL838X_PRI_SEL_TBL_CTRL(i)		(0x5fd8 + (((i) << 2)))
#define RTL838X_QM_INTPRI2QID_CTRL		(0x5f00)
#define RTL838X_QM_PKT2CPU_INTPRI_0		(0x5f04)
#define RTL838X_QM_PKT2CPU_INTPRI_1		(0x5f08)
#define RTL838X_QM_PKT2CPU_INTPRI_2		(0x5f0c)
#define RTL838X_QM_PKT2CPU_INTPRI_MAP		(0x5f10)
#define RTL838X_RMK_IPRI_CTRL			(0xa460)
#define RTL838X_RMK_OPRI_CTRL			(0xa464)
#define RTL838X_SCHED_LB_CTRL(p)		(0xc004 + (((p) << 7)))
#define RTL838X_SCHED_P_TYPE_CTRL(p)		(0xc04c + (((p) << 7)))

/* Packet Inspection Engine */
#define RTL838X_ACL_BLK_GROUP_CTRL		(0x615c)
#define RTL838X_ACL_BLK_LOOKUP_CTRL		(0x6100)
#define RTL838X_ACL_BLK_PWR_CTRL		(0x6104)
#define RTL838X_ACL_BLK_TMPLTE_CTRL(block)	(0x6108 + ((block) << 2))
#define RTL838X_ACL_CLR_CTRL			(0x6168)
#define RTL838X_ACL_PORT_LOOKUP_CTRL(p)		(0x616c + (((p) << 2)))
#define RTL838X_DMY_REG27			(0x3378)
#define RTL838X_METER_GLB_CTRL			(0x4b08)

/* Misc Registers definitions */
#define RTL838X_CHIP_INFO			(0x00d8)
#define RTL838X_DMY_REG31			(0x3b28)
#define RTL838X_INT_MODE_CTRL			(0x005c)

/* Interrupt control */
#define RTL838X_IMR_GLB_REG                             (0x1100)
/* Reserved                                                     31 - 1 */
#define RTL838X_IMR_GLB_SWITCH                                  BIT(0)

#define RTL838X_IMR_PORT_LINK_STS_REG(p)                (0x1104 + (((p) / 32) * 0x4))
#define _RTL838X_IMR_PORT_LINK_STS_MASK                         BIT(0)
#define RTL838X_IMR_PORT_LINK_STS(p) \
        (_RTL838X_IMR_PORT_LINK_STS_MASK << ((p) % 32))

#define RTL838X_IMR_PORT_MEDIA_STS_REG(p)               (0x1108 + (((p) / 32) * 0x4))
/* Reserved                                                     31 - 4 */
#define _RTL838X_IMR_PORT_MEDIA_STS_CHG_MASK                    BIT(0)
#define RTL838X_IMR_PORT_MEDIA_STS_CHG(p) \
        (_RTL838X_IMR_PORT_MEDIA_STS_CHG_MASK << ((p) % 32))

#define RTL838X_ISR_GLB_SRC_REG                         (0x1148)
/* Reserved                                                     31 - 15 */
#define RTL838X_ISR_GLB_SRC_SYSCLK_GATE                         BIT(14)
#define RTL838X_ISR_GLB_SRC_TM                                  BIT(13)
#define RTL838X_ISR_GLB_SRC_EXTRA_GPIO                          BIT(12)
#define RTL838X_ISR_GLB_SRC_INT_GPHY                            BIT(11)
#define RTL838X_ISR_GLB_SRC_GPIO                                BIT(10)
#define RTL838X_ISR_GLB_SRC_ACL_HIT                             BIT(9)
#define RTL838X_ISR_GLB_SRC_FID_SALRN_CONSTRT                   BIT(8)
#define RTL838X_ISR_GLB_SRC_TIMESTAMP_LATCH                     BIT(7)
#define RTL838X_ISR_GLB_SRC_SERDES5                             BIT(6)
#define RTL838X_ISR_GLB_SRC_SERDES4                             BIT(5)
#define RTL838X_ISR_GLB_SRC_SERDES23                            BIT(4)
#define RTL838X_ISR_GLB_SRC_SERDES01                            BIT(3)
#define RTL838X_ISR_GLB_SRC_SALARN_CONSTRT                      BIT(2)
#define RTL838X_ISR_GLB_SRC_MEDIA_CHG                           BIT(1)
#define RTL838X_ISR_GLB_SRC_LINK_CHG                            BIT(0)

#define RTL838X_ISR_PORT_MEDIA_STS_REG(p)               (0x1150 + (((p) / 32) * 0x4))
/* Reserved                                                     31 - 4 */
#define _RTL838X_ISR_PORT_MEDIA_STS_CHG_MASK                    BIT(0)
#define RTL838X_ISR_PORT_MEDIA_STS_CHG(p, r) \
        (((r) >> ((p) % 32)) & _RTL838X_ISR_PORT_MEDIA_STS_CHG_MASK)
#define RTL838X_ISR_PORT_MEDIA_STS_CHG_CLR(p) \
        (_RTL838X_ISR_PORT_MEDIA_STS_CHG_MASK << ((p) % 32))

#define RTL838X_ISR_PORT_LINK_STS_REG(p)                (0x114c + ((p / 32) * 0x4))
#define _RTL838X_ISR_PORT_LINK_STS_MASK                         BIT(0)
#define RTL838X_ISR_PORT_LINK_STS(p, r) \
        (((r) >> ((p) % 32)) & _RTL838X_ISR_PORT_LINK_STS_MASK)
#define RTL838X_ISR_PORT_LINK_STS_CLR(p) \
        (_RTL838X_ISR_PORT_LINK_STS_MASK << ((p) % 32))

void rtl838x_dbgfs_init(struct rtl838x_switch_priv *priv);
void rtl8380_get_version(struct rtl838x_switch_priv *priv);
u32 rtl838x_hash(struct rtl838x_switch_priv *priv, u64 seed);
void rtl838x_print_matrix(void);
void rtl838x_imr_port_link_sts_chg(const u64 ports);
void rtl838x_imr_port_media_sts_chg(const u64 ports);
void rtl838x_isr_port_link_sts_chg(const u64 ports);
void rtl838x_isr_port_media_sts_chg(const u64 ports);
int rtl838x_mac_link_dup_sts(const int port);
int rtl838x_mac_link_media_sts(const int port);
int rtl838x_mac_link_spd_sts(const int port);
int rtl838x_mac_link_sts(const int port);
int rtl838x_mac_rx_pause_sts(const int port);
int rtl838x_mac_tx_pause_sts(const int port);
int rtl8380_sds_power(int mac, int val);
void rtl8380_sds_rst(int mac);
irqreturn_t rtl838x_switch_irq(int irq, void *dev_id);
void rtl838x_vlan_profile_dump(int index);

#endif /* _RTL838X_H */
