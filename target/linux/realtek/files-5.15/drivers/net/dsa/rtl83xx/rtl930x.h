/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _RTL930X_H
#define _RTL930X_H __FILE__

#include <linux/bitops.h>
#include <linux/types.h>
#include <linux/phy.h>
#include <linux/phylink.h>

#include "rtl83xx.h"

/* MAC port control */
#define RTL930X_MAC_PORT_CTRL_REG(p)                    (0x3260 + ((p) * 0x40))
/* Reserved                                                     31 - 5 */
#define RTL930X_MAC_PORT_CTRL_PRECOLLAT_SEL                     GENMASK(4, 3)
#define RTL930X_MAC_PORT_CTRL_LATE_COLI_THR                     GENMASK(2, 1)
#define RTL930X_MAC_PORT_CTRL_BKPRES_EN                         BIT(0)

#define RTL930X_MAC_L2_PORT_CTRL_REG(p)                 (0x3268 + ((p) * 0x40))
/* Reserved                                                     31 - 6 */
#define RTL930X_MAC_L2_PORT_CTRL_PADDING_UND_SIZE_EN            BIT(5)
#define RTL930X_MAC_L2_PORT_CTRL_RX_CHK_CRC_EN                  BIT(4)
#define RTL930X_MAC_L2_PORT_CTRL_PASS_ALL_MODE_EN               BIT(3)
#define RTL930X_MAC_L2_PORT_CTRL_BYP_TX_CRC                     BIT(2)
#define RTL930X_MAC_L2_PORT_CTRL_TX_EN                          BIT(1)
#define RTL930X_MAC_L2_PORT_CTRL_RX_EN                          BIT(0)
#define RTL930X_MAC_L2_PORT_CTRL_TXRX_EN \
        (RTL930X_MAC_L2_PORT_CTRL_TX_EN | RTL930X_MAC_L2_PORT_CTRL_RX_EN)

#define RTL930X_MAC_FORCE_MODE_CTRL_REG(p)              (0xca1c + ((p) * 0x4))
/* Reserved                                                     31 - 18 */
#define RTL930X_MAC_FORCE_MODE_CTRL_BYP_LINK                    BIT(17)
#define RTL930X_MAC_FORCE_MODE_CTRL_MEDIA_SEL                   BIT(16)
#define RTL930X_MAC_FORCE_MODE_CTRL_EEE_EN_10G                  BIT(15)
#define RTL930X_MAC_FORCE_MODE_CTRL_EEE_EN_5G                   BIT(14)
#define RTL930X_MAC_FORCE_MODE_CTRL_EEE_EN_2G5                  BIT(13)
#define RTL930X_MAC_FORCE_MODE_CTRL_EEE_EN_1000M                BIT(12)
#define RTL930X_MAC_FORCE_MODE_CTRL_EEE_EN_500M                 BIT(11)
#define RTL930X_MAC_FORCE_MODE_CTRL_EEE_EN_100M                 BIT(10)
#define RTL930X_MAC_FORCE_MODE_CTRL_FC_EN                       BIT(9)
#define RTL930X_MAC_FORCE_MODE_CTRL_RX_PAUSE_EN                 BIT(8)
#define RTL930X_MAC_FORCE_MODE_CTRL_TX_PAUSE_EN                 BIT(7)
#define RTL930X_MAC_FORCE_MODE_CTRL_SPD_SEL                     GENMASK(6, 3)
#define RTL930X_MAC_FORCE_MODE_CTRL_SPD_SEL_5G                          0x6
#define RTL930X_MAC_FORCE_MODE_CTRL_SPD_SEL_2G5                         0x5
#define RTL930X_MAC_FORCE_MODE_CTRL_SPD_SEL_10G                         0x4
#define RTL930X_MAC_FORCE_MODE_CTRL_SPD_SEL_500M                        0x3
#define RTL930X_MAC_FORCE_MODE_CTRL_SPD_SEL_1000M                       0x2
#define RTL930X_MAC_FORCE_MODE_CTRL_SPD_SEL_100M                        0x1
#define RTL930X_MAC_FORCE_MODE_CTRL_SPD_SEL_10M                         0x0
#define RTL930X_MAC_FORCE_MODE_CTRL_DUP_SEL                     BIT(2)
#define RTL930X_MAC_FORCE_MODE_CTRL_LINK_EN                     BIT(1)
#define RTL930X_MAC_FORCE_MODE_CTRL_EN                          BIT(0)

#define RTL930X_MAC_FORCE_MODE_CTRL		(0xca1c)
#define RTL930X_MAC_L2_PORT_CTRL(port)		(0x3268 + (((port) << 6)))
#define RTL930X_MAC_PORT_CTRL(port)		(0x3260 + (((port) << 6)))

/* Packet statistics */
#define RTL930X_STAT_CTRL			(0x3248)
#define RTL930X_STAT_PORT_MIB_CNTR		(0x0664)
#define RTL930X_STAT_PORT_RST			(0x3244)
#define RTL930X_STAT_RST			(0x3240)

/* VLAN registers */
#define RTL930X_VLAN_CTRL			(0x82d4)
#define RTL930X_VLAN_PORT_EGR_FLTR		(0x83c8)
#define RTL930X_VLAN_PORT_FWD			(0x834c)
#define RTL930X_VLAN_PORT_IGR_FLTR		(0x83c0)
#define RTL930X_VLAN_PORT_PB_VLAN		(0x82d8)
#define RTL930X_VLAN_PROFILE_SET(idx)		(0x9c60 + (((idx) * 20)))

/* Table access registers */
#define RTL930X_TBL_ACCESS_CTRL_0		(0xb340)
#define RTL930X_TBL_ACCESS_DATA_0(idx)		(0xb344 + ((idx) << 2))
#define RTL930X_TBL_ACCESS_CTRL_1		(0xb3a0)
#define RTL930X_TBL_ACCESS_DATA_1(idx)		(0xb3a4 + ((idx) << 2))
#define RTL930X_TBL_ACCESS_CTRL_2		(0xce04)
#define RTL930X_TBL_ACCESS_DATA_2(i)		(0xce08 + (((i) << 2)))

/* MAC handling */
#define RTL930X_MAC_LINK_DUP_STS_REG(p)                 (0xcb28 + (((p) / 32) * 0x4))
#define _RTL930X_MAC_LINK_DUP_STS_MASK                          BIT(0)
#define RTL930X_MAC_LINK_DUP_STS_FULL                                   0b1
#define RTL930X_MAC_LINK_DUP_STS_HALF                                   0b0
#define RTL930X_MAC_LINK_DUP_STS(p, r) \
        (((r) >> ((p) % 32)) & _RTL930X_MAC_LINK_DUP_STS_MASK)

#define RTL930X_MAC_LINK_MEDIA_STS_REG(p)               (0xcb14 + (((p) / 32) * 0x4))
#define _RTL930X_MAC_LINK_MEDIA_STS_MASK                        BIT(0)
#define RTL930X_MAC_LINK_MEDIA_STS_FIBER                                0b1
#define RTL930X_MAC_LINK_MEDIA_STS_COPPER                               0b0
#define RTL930X_MAC_LINK_MEDIA_STS(p, r) \
        (((r) >> ((p) % 32)) & _RTL930X_MAC_LINK_MEDIA_STS_MASK)

#define RTL930X_MAC_LINK_SPD_STS_REG(p)                 (0xcb18 + (((p) / 8) * 0x4))
#define _RTL930X_MAC_LINK_SPD_STS_MASK                          GENMASK(3, 0)
#define RTL930X_MAC_LINK_SPD_STS_2G5_ALT                                0x8
#define RTL930X_MAC_LINK_SPD_STS_1000M_ALT                              0x7
#define RTL930X_MAC_LINK_SPD_STS_5G                                     0x6
#define RTL930X_MAC_LINK_SPD_STS_2G5                                    0x5
#define RTL930X_MAC_LINK_SPD_STS_10G                                    0x4
#define RTL930X_MAC_LINK_SPD_STS_500M                                   0x3
#define RTL930X_MAC_LINK_SPD_STS_1000M                                  0x2
#define RTL930X_MAC_LINK_SPD_STS_100M                                   0x1
#define RTL930X_MAC_LINK_SPD_STS_10M                                    0x0
#define RTL930X_MAC_LINK_SPD_STS(p, r) \
        (((r) >> (((p) % 8) * 4)) & _RTL930X_MAC_LINK_SPD_STS_MASK)

#define RTL930X_MAC_LINK_STS_REG(p)                     (0xcb10 + (((p) / 32) * 0x4))
#define RTL930X_MAC_LINK_STS_MASK                               BIT(0)
#define RTL930X_MAC_LINK_STS_UP                                         0b1
#define RTL930X_MAC_LINK_STS_DOWN                                       0b0
#define RTL930X_MAC_LINK_STS(p, r) \
        (((r) >> ((p) % 32)) & RTL930X_MAC_LINK_STS_MASK)

#define RTL930X_MAC_RX_PAUSE_STS_REG(p)                 (0xcb30 + (((p) / 32) * 0x4))
#define _RTL930X_MAC_RX_PAUSE_STS_MASK                          BIT(0)
#define RTL930X_MAC_RX_PAUSE_STS_ON                                     0b1
#define RTL930X_MAC_RX_PAUSE_STS_OFF                                    0b0
#define RTL930X_MAC_RX_PAUSE_STS(p, r) \
        (((r) >> ((p) % 32)) & _RTL930X_MAC_RX_PAUSE_STS_MASK)

#define RTL930X_MAC_TX_PAUSE_STS_REG(p)                 (0xcb2c + (((p) / 32) * 0x4))
#define _RTL930X_MAC_TX_PAUSE_STS_MASK                          BIT(0)
#define RTL930X_MAC_TX_PAUSE_STS_ON                                     0b1
#define RTL930X_MAC_TX_PAUSE_STS_OFF                                    0b0
#define RTL930X_MAC_TX_PAUSE_STS(p, r) \
        (((r) >> ((p) % 32)) & _RTL930X_MAC_TX_PAUSE_STS_MASK)

#define RTL930X_MAC_LINK_DUP_STS_ADDR		(0xcb28)
#define RTL930X_MAC_LINK_MEDIA_STS_ADDR		(0xcb14)
#define RTL930X_MAC_LINK_SPD_STS_PORT_ADDR(p)	(0xcb18 + (((p >> 3) << 2)))
#define RTL930X_MAC_LINK_STS_ADDR		(0xcb10)
#define RTL930X_MAC_RX_PAUSE_STS_ADDR		(0xcb30)
#define RTL930X_MAC_TX_PAUSE_STS_ADDR		(0xcb2c)

#define RTL930X_FORCE_EN			(1 << 0)
#define RTL930X_FORCE_LINK_EN			(1 << 1)
#define RTL930X_DUPLEX_MODE			(1 << 2)
#define RTL930X_TX_PAUSE_EN			(1 << 7)
#define RTL930X_RX_PAUSE_EN			(1 << 8)
#define RTL930X_MAC_FORCE_FC_EN			(1 << 9)

/* EEE */
#define RTL930X_MAC_EEE_ABLTY			(0xcb34)
#define RTL930X_EEE_CTRL(p)			(0x3274 + ((p) << 6))
#define RTL930X_EEEP_PORT_CTRL(p)		(0x3278 + ((p) << 6))

/* L2 functionality */
#define	RTL930X_L2_PORT_AGE_CTRL		(0x8fe0)
#define RTL930X_L2_AGE_CTRL			(0x8fdc)
#define RTL930X_L2_BC_FLD_PMSK			(0x9068)
#define RTL930X_L2_CTRL				(0x8fd8)
#define RTL930X_L2_LRN_CONSTRT_CTRL		(0x909c)
#define RTL930X_L2_PORT_DABLK_CTRL		(0x9060)
#define RTL930X_L2_PORT_NEW_SA_FWD(p)		(0x8ff4 + (((p / 10) << 2)))
#define RTL930X_L2_PORT_SABLK_CTRL		(0x905c)
#define RTL930X_L2_PORT_SALRN(p)		(0x8fec + (((p >> 4) << 2)))
#define RTL930X_L2_TBL_FLUSH_CTRL		(0x9404)
#define RTL930X_L2_UNKN_UC_FLD_PMSK		(0x9064)
#define RTL930X_ST_CTRL				(0x8798)
#define RTL930X_TBL_ACCESS_L2_CTRL		(0xb320)
#define RTL930X_TBL_ACCESS_L2_DATA(idx)		(0xab08 + ((idx) << 2))
#define RTL930X_TBL_ACCESS_L2_METHOD_CTRL	(0xb324)

/* Port Mirroring */
#define RTL930X_MIR_CTRL			(0xa2a0)
#define RTL930X_MIR_DPM_CTRL			(0xa2c0)
#define RTL930X_MIR_SPM_CTRL			(0xa2b0)

/* Link aggregation (Trunking) */
#define RTL930X_TRK_HASH_CTRL			(0x9f80)
#define RTL930X_TRK_MBR_CTRL			(0xa41c)

/* 802.1X */
#define RTL930X_RMA_BPDU_CTRL			(0x9e7c)
#define RTL930X_RMA_BPDU_FLD_PMSK		(0x9f18)
#define RTL930X_RMA_EAPOL_CTRL			(0x9f08)
#define RTL930X_RMA_LLTP_CTRL			(0x9efc)
#define RTL930X_RMA_PTP_CTRL			(0x9e88)

/* Debug features */
#define RTL930X_STAT_PRVTE_DROP_COUNTER0	(0xb5b8)

/* Packet Inspection Engine */
#define RTL930X_ACL_PORT_LOOKUP_CTRL(p)		(0xa784 + (((p) << 2)))
#define RTL930X_METER_GLB_CTRL			(0xa0a0)
#define RTL930X_PIE_BLK_LOOKUP_CTRL		(0xa5a0)
#define RTL930X_PIE_BLK_PHASE_CTRL		(0xa5a4)
#define RTL930X_PIE_BLK_TMPLTE_CTRL(block)	(0xa624 + ((block) << 2))
#define RTL930X_PIE_CLR_CTRL			(0xa66c)

/* L3 Routing */
#define RTL930X_L3_HOST_TBL_CTRL		(0xab48)
#define RTL930X_L3_HW_LU_CTRL			(0xacc0)

#define RTL930X_L3_HW_LU_KEY_CTRL                       (0xac9c)
#define _RTL930X_L3_HW_LU_KEY_CTRL_ROUTE_TYPE_MASK              GENMASK(20, 19)
#define RTL930X_L3_HW_LU_KEY_CTRL_ROUTE_TYPE_OFFSET             19
#define RTL930X_L3_HW_LU_KEY_CTRL_ROUTE_TYPE(r) \
        (((r) << RTL930X_L3_HW_LU_KEY_CTRL_ROUTE_TYPE_OFFSET) & _RTL930X_L3_HW_LU_KEY_CTRL_ROUTE_TYPE_MASK)
#define RTL930X_L3_HW_LU_KEY_CTRL_ROUTE_TYPE_IPV6MC                     0x3
#define RTL930X_L3_HW_LU_KEY_CTRL_ROUTE_TYPE_IPV6UC                     0x2
#define RTL930X_L3_HW_LU_KEY_CTRL_ROUTE_TYPE_IPV4MC                     0x1
#define RTL930X_L3_HW_LU_KEY_CTRL_ROUTE_TYPE_IPV4UC                     0x0

#define RTL930X_L3_HW_LU_KEY_IP_CTRL		(0xaca0)
#define RTL930X_L3_IP6MC_ROUTE_CTRL		(0xab58)
#define RTL930X_L3_IP6UC_ROUTE_CTRL		(0xab50)
#define RTL930X_L3_IP6_MTU_CTRL(i)		(0xab6c + ((i >> 1) << 2))
#define RTL930X_L3_IPMC_ROUTE_CTRL		(0xab54)
#define RTL930X_L3_IPUC_ROUTE_CTRL		(0xab4c)
#define RTL930X_L3_IP_MTU_CTRL(i)		(0xab5c + ((i >> 1) << 2))
#define RTL930X_L3_IP_ROUTE_CTRL		(0xab44)

/* Port LED Control */
#define RTL930X_LED_PORT_NUM_CTRL(p)		(0xcc04 + (((p >> 4) << 2)))
#define RTL930X_LED_SET0_0_CTRL			(0xcc28)
#define RTL930X_LED_PORT_COPR_SET_SEL_CTRL(p)	(0xcc2c + (((p >> 4) << 2)))
#define RTL930X_LED_PORT_FIB_SET_SEL_CTRL(p)	(0xcc34 + (((p >> 4) << 2)))
#define RTL930X_LED_PORT_COPR_MASK_CTRL		(0xcc3c)
#define RTL930X_LED_PORT_FIB_MASK_CTRL		(0xcc40)
#define RTL930X_LED_PORT_COMBO_MASK_CTRL	(0xcc44)

/* Interrupt control */
#define RTL930X_IMR_GLB_REG                             (0xc628)
/* Reserved                                                     31 - 1 */
#define RTL930X_IMR_GLB_EXT_CPU                               BIT(0)

#define RTL930X_IMR_PORT_LINK_STS_REG(p)                (0xc62c + (((p) / 32) * 0x4))
#define _RTL930X_IMR_PORT_LINK_STS_MASK                         BIT(0)
#define RTL930X_IMR_PORT_LINK_STS(p) \
        (_RTL930X_IMR_PORT_LINK_STS_MASK << ((p) % 32))

#define RTL930X_IMR_SERDES_LINK_FAULT_REG               (0xc634)
#define _RTL930X_IMR_SERDES_LINK_FAULT_MASK                     BIT(0)
#define RTL930X_IMR_SERDES_LINK_FAULT(p) \
        (_RTL930X_IMR_SERDES_LINK_FAULT_MASK << (p))

#define RTL930X_IMR_SERDES_RX_SYM_ERR_REG               (0xc638)
#define _RTL930X_IMR_SERDES_RX_SYM_ERR_MASK                     BIT(0)
#define RTL930X_IMR_SERDES_RX_SYM_ERR(r) \
        (_RTL930X_IMR_SERDES_RX_SYM_ERR_MASK << (p))

#define RTL930X_IMR_SERDES_UPD_PHY_STS_REG(p)           (0xc650 + (((p) / 32) * 0x4))
#define _RTL930X_IMR_SERDES_UPD_PHY_STS_MASK                    BIT(0)
#define RTL930X_IMR_SERDES_UPD_PHY_STS(p) \
        (_RTL930X_IMR_SERDES_UPD_PHY_STS_MASK << ((p) % 32))

#define RTL930X_ISR_GLB_REG                             (0xc658)
/* Reserved                                                     31 - 22 */
#define RTL930X_ISR_GLB_SDS_RX_SYM_ERR                          BIT(21)
#define RTL930X_ISR_GLB_ROUT_L2_NTFY_BUF                        BIT(20)
#define RTL930X_ISR_GLB_ROUT_PBUF                               BIT(19)
#define RTL930X_ISR_GLB_RLFD                                    BIT(18)
#define RTL930X_ISR_GLB_SDS_UPD_PHY_STS                         BIT(17)
#define RTL930X_ISR_GLB_AUTO_REC                                BIT(16)
/* Reserved                                                     15 */
#define RTL930X_ISR_GLB_SMI_CHECK                               BIT(14)
#define RTL930X_ISR_GLB_TERMAL_DETECT                           BIT(13)
#define RTL930X_ISR_GLB_EXT_GPIO                                BIT(12)
/* Reserved                                                     11 */
#define RTL930X_ISR_GLB_OAM_DYGASP                              BIT(10)
/* Reserved                                                     9 - 3 */
#define RTL930X_ISR_GLB_SERDES_LINK_FAULT_P                     BIT(2)
/* Reserved                                                     1 */
#define RTL930X_ISR_GLB_LINK_CHG                                BIT(0)

#define RTL930X_ISR_PORT_LINK_STS_REG(p)                (0xc660 + ((p / 32) * 0x4))
#define _RTL930X_ISR_PORT_LINK_STS_MASK                         BIT(0)
#define RTL930X_ISR_PORT_LINK_STS(p, r) \
        (((r) >> ((p) % 32)) & _RTL930X_ISR_PORT_LINK_STS_MASK)
#define RTL930X_ISR_PORT_LINK_STS_CLR(p) \
        (_RTL930X_ISR_PORT_LINK_STS_MASK << ((p) % 32))

#define RTL930X_ISR_SERDES_LINK_FAULT_REG               (0xc668)
#define _RTL930X_ISR_SERDES_LINK_FAULT_MASK                     BIT(0)
#define RTL930X_ISR_SERDES_LINK_FAULT(p, r) \
        (((r) >> (p)) & _RTL930X_ISR_SERDES_LINK_FAULT_MASK)
#define RTL930X_ISR_SERDES_LINK_FAULT_CLR(p) \
        (_RTL930X_ISR_SERDES_LINK_FAULT_MASK << (p))

#define RTL930X_ISR_SERDES_RX_SYM_ERR_REG               (0xc66c)
#define _RTL930X_ISR_SERDES_RX_SYM_ERR_MASK                     BIT(0)
#define RTL930X_ISR_SERDES_RX_SYM_ERR(p, r) \
        (((r) >> (p)) & _RTL930X_ISR_SERDES_RX_SYM_ERR_MASK)
#define RTL930X_ISR_SERDES_RX_SYM_ERR_CLR(r) \
        (_RTL930X_ISR_SERDES_RX_SYM_ERR_MASK << (p))

#define RTL930X_ISR_SERDES_UPD_PHY_STS_REG(p)           (0xc690 + ((p) * 0x4))
#define _RTL930X_ISR_SERDES_UPD_PHY_STS_MASK                    BIT(0)
#define RTL930X_ISR_SERDES_UPD_PHY_STS(p, r) \
        (((r) >> ((p) % 32)) & _RTL930X_ISR_SERDES_UPD_PHY_STS_MASK)
#define RTL930X_ISR_SERDES_UPD_PHY_STS_CLR(r) \
        (_RTL930X_ISR_SERDES_UPD_PHY_STS_MASK << ((p) % 32))

int rtl9300_configure_serdes(const int port,
                             const enum phylink_port,
                             const unsigned int link_length,
                             const int sds_num,
                             phy_interface_t phy_mode);
void rtl930x_dbgfs_init(struct rtl838x_switch_priv *priv);
u32 rtl930x_hash(struct rtl838x_switch_priv *priv, u64 seed);
void rtl930x_imr_port_link_sts_chg(const u64 ports);
void rtl930x_isr_port_link_sts_chg(const u64 ports);
int rtl930x_mac_force_mode_ctrl(const int port);
int rtl930x_mac_link_dup_sts(const int port);
int rtl930x_mac_link_media_sts(const int port);
int rtl930x_mac_link_spd_sts(const int port);
int rtl930x_mac_link_sts(const int port);
int rtl930x_mac_port_ctrl(const int port);
int rtl930x_mac_rx_pause_sts(const int port);
int rtl930x_mac_tx_pause_sts(const int port);
void rtl930x_print_matrix(void);
int rtl9300_rtl8226_mode_set(int port, int sds_num, phy_interface_t phy_mode);
u32 rtl9300_sds_field_r(int sds, u32 page, u32 reg, int end_bit, int start_bit);
void rtl9300_sds_field_w(int sds, u32 page, u32 reg, int end_bit, int start_bit, u32 v);
int rtl9300_sds_power(int mac, int val);
void rtl9300_sds_rst(int sds_num, u32 mode);
int rtl9300_serdes_setup(int sds_num, phy_interface_t phy_mode);
irqreturn_t rtl930x_switch_irq(int irq, void *dev_id);
void rtl930x_vlan_profile_dump(int index);

#endif /* _RTL930X_H */
