/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _RTL931X_H
#define _RTL931X_H __FILE__

#include <linux/bitops.h>
#include <linux/types.h>
#include <linux/phy.h>

/* MAC port control */
#define RTL931X_MAC_FORCE_MODE_CTRL_REG(p)              (0x0dcc + ((p) * 0x4))
/* Reserved                                                     31 - 28 */
#define RTL931X_MAC_FORCE_MODE_CTRL_SDS_ABLTY                   BIT(27)
/* Reserved                                                     26 */
#define RTL931X_MAC_FORCE_MODE_CTRL_FEFI_SEL                    BIT(25)
#define RTL931X_MAC_FORCE_MODE_CTRL_MSTR_SLV                    BIT(24)
#define RTL931X_MAC_FORCE_MODE_CTRL_EEE_10G_EN                  BIT(23)
#define RTL931X_MAC_FORCE_MODE_CTRL_EEE_5G_EN                   BIT(22)
#define RTL931X_MAC_FORCE_MODE_CTRL_EEE_2G5_EN                  BIT(21)
#define RTL931X_MAC_FORCE_MODE_CTRL_EEE_1000M_EN                BIT(20)
#define RTL931X_MAC_FORCE_MODE_CTRL_EEE_500M_EN                 BIT(19)
#define RTL931X_MAC_FORCE_MODE_CTRL_EEE_100M_EN                 BIT(18)
#define RTL931X_MAC_FORCE_MODE_CTRL_RX_PAUSE_EN                 BIT(17)
#define RTL931X_MAC_FORCE_MODE_CTRL_TX_PAUSE_EN                 BIT(16)
#define RTL931X_MAC_FORCE_MODE_CTRL_SPD_SEL                     GENMASK(15, 12)
#define RTL931X_MAC_FORCE_MODE_CTRL_SPD_SEL_5G                          0x6
#define RTL931X_MAC_FORCE_MODE_CTRL_SPD_SEL_2G5                         0x5
#define RTL931X_MAC_FORCE_MODE_CTRL_SPD_SEL_10G                         0x4
#define RTL931X_MAC_FORCE_MODE_CTRL_SPD_SEL_500M                        0x3
#define RTL931X_MAC_FORCE_MODE_CTRL_SPD_SEL_1000M                       0x2
#define RTL931X_MAC_FORCE_MODE_CTRL_SPD_SEL_100M                        0x1
#define RTL931X_MAC_FORCE_MODE_CTRL_SPD_SEL_10M                         0x0
#define RTL931X_MAC_FORCE_MODE_CTRL_DUP_SEL                     BIT(11)
#define RTL931X_MAC_FORCE_MODE_CTRL_MEDIA                       BIT(10)
#define RTL931X_MAC_FORCE_MODE_CTRL_EN                          BIT(9)
/* Reserved                                                     8 */
#define RTL931X_MAC_FORCE_MODE_CTRL_FEFI_EN                     BIT(7)
#define RTL931X_MAC_FORCE_MODE_CTRL_MSTR_SLV_EN                 BIT(6)
#define RTL931X_MAC_FORCE_MODE_CTRL_EEE_EN                      BIT(5)
#define RTL931X_MAC_FORCE_MODE_CTRL_FC_EN                       BIT(4)
#define RTL931X_MAC_FORCE_MODE_CTRL_SPD_EN                      BIT(3)
#define RTL931X_MAC_FORCE_MODE_CTRL_DUP_EN                      BIT(2)
#define RTL931X_MAC_FORCE_MODE_CTRL_MEDIA_EN                    BIT(1)
#define RTL931X_MAC_FORCE_MODE_CTRL_LINK_EN                     BIT(0)

#define RTL931X_MAC_L2_PORT_CTRL_REG(p)                 (0x6000 + ((p) * 0x80))
/* Reserved                                                     31 - 21 */
#define RTL931X_MAC_L2_PORT_CTRL_PER_PORT_MAC_ECO               BIT(22)
#define RTL931X_MAC_L2_PORT_CTRL_STK_1G_PKT_FMT                 BIT(21)
#define RTL931X_MAC_L2_PORT_CTRL_TX_IPG                         GENMASK(20, 5)
#define RTL931X_MAC_L2_PORT_CTRL_PADDING_UND_SIZE_EN            BIT(4)
#define RTL931X_MAC_L2_PORT_CTRL_PASS_ALL_MODE_EN               BIT(3)
#define RTL931X_MAC_L2_PORT_CTRL_BYP_TX_CRC                     BIT(2)
#define RTL931X_MAC_L2_PORT_CTRL_TX_EN                          BIT(1)
#define RTL931X_MAC_L2_PORT_CTRL_RX_EN                          BIT(0)
#define RTL931X_MAC_L2_PORT_CTRL_TXRX_EN \
        (RTL931X_MAC_L2_PORT_CTRL_TX_EN | RTL931X_MAC_L2_PORT_CTRL_RX_EN)

#define RTL931X_MAC_PORT_CTRL_REG(p)                    (0x6004 + ((p) * 0x80))
/* Reserved                                                     31 - 5 */
#define RTL931X_MAC_PORT_CTRL_PRECOLLAT_SEL                     GENMASK(4, 3)
#define RTL931X_MAC_PORT_CTRL_LATE_COLI_THR                     GENMASK(2 ,1)
#define RTL931X_MAC_PORT_CTRL_BKPRES_EN                         BIT(0)

#define RTL931X_PORT_ISO_VB_ISO_PMSK_CTRL(port)	(0x8a04 + ((port) << 2))

#define RTL931X_MAC_EEE_ABLTY_REG(p)                    (0x0f08 + (((p) / 32) * 0x4))
#define _RTL931X_MAC_EEE_ABLTY_ABLE_MASK                        BIT(0)
#define RTL931X_MAC_EEE_ABLTY_ABLE_ON                                   0b1
#define RTL931X_MAC_EEE_ABLTY_ABLE_OFF                                  0b0
#define RTL931X_MAC_EEE_ABLTY_ABLE(r, p) \
        (((r) >> ((p) % 32)) & _RTL931X_MAC_EEE_ABLTY_ABLE_MASK)

/* Packet statistics */
#define RTL931X_STAT_CTRL			(0x5720)
#define RTL931X_STAT_PORT_RST			(0x7ef8)
#define RTL931X_STAT_RST			(0x7ef4)

/* VLAN registers */
#define RTL931X_VLAN_CTRL			(0x94e4)
#define RTL931X_VLAN_PORT_EGR_FLTR		(0x96c4)
#define RTL931X_VLAN_PORT_FWD			(0x95cc)
#define RTL931X_VLAN_PORT_IGR_CTRL		(0x94e8)
#define RTL931X_VLAN_PORT_IGR_FLTR		(0x96b4)
#define RTL931X_VLAN_PROFILE_SET(idx)		(0x9800 + (((idx) * 28)))

/* Table access registers */
#define RTL931X_TBL_ACCESS_CTRL_0		(0x8500)
#define RTL931X_TBL_ACCESS_DATA_0(idx)		(0x8508 + ((idx) << 2))
#define RTL931X_TBL_ACCESS_CTRL_1		(0x40c0)
#define RTL931X_TBL_ACCESS_DATA_1(idx)		(0x40c4 + ((idx) << 2))
#define RTL931X_TBL_ACCESS_CTRL_2		(0x8528)
#define RTL931X_TBL_ACCESS_DATA_2(i)		(0x852c + (((i) << 2)))
#define RTL931X_TBL_ACCESS_CTRL_3		(0x0200)
#define RTL931X_TBL_ACCESS_DATA_3(i)		(0x0204 + (((i) << 2)))
#define RTL931X_TBL_ACCESS_CTRL_4		(0x20dc)
#define RTL931X_TBL_ACCESS_DATA_4(i)		(0x20e0 + (((i) << 2)))
#define RTL931X_TBL_ACCESS_CTRL_5		(0x7e1c)
#define RTL931X_TBL_ACCESS_DATA_5(i)		(0x7e20 + (((i) << 2)))

/* MAC handling */
#define RTL931X_MAC_LINK_DUP_STS_REG(p)                 (0x0ef0 + (((p) / 32) * 0x4))
#define _RTL931X_MAC_LINK_DUP_STS_MASK                          BIT(0)
#define RTL931X_MAC_LINK_DUP_STS_FULL                                   0b1
#define RTL931X_MAC_LINK_DUP_STS_HALF                                   0b0
#define RTL931X_MAC_LINK_DUP_STS(p, r) \
        (((r) >> ((p) % 32)) & _RTL931X_MAC_LINK_DUP_STS_MASK)

#define RTL931X_MAC_LINK_MEDIA_STS_REG(p)               (0x0ec8 + (((p) / 32) * 0x4))
#define _RTL931X_MAC_LINK_MEDIA_STS_MASK                        BIT(0)
#define RTL931X_MAC_LINK_MEDIA_STS_FIBER                                0b1
#define RTL931X_MAC_LINK_MEDIA_STS_COPPER                               0b0
#define RTL931X_MAC_LINK_MEDIA_STS(p, r) \
        (((r) >> ((p) % 32)) & _RTL931X_MAC_LINK_MEDIA_STS_MASK)

#define RTL931X_MAC_LINK_SPD_STS_REG(p)                 (0x0ed0 + (((p) / 8) * 0x4))
#define _RTL931X_MAC_LINK_SPD_STS_MASK                          GENMASK(3, 0)
#define RTL931X_MAC_LINK_SPD_STS_2G5_ALT                                0x8
#define RTL931X_MAC_LINK_SPD_STS_1000M_ALT                              0x7
#define RTL931X_MAC_LINK_SPD_STS_5G                                     0x6
#define RTL931X_MAC_LINK_SPD_STS_2G5                                    0x5
#define RTL931X_MAC_LINK_SPD_STS_10G                                    0x4
#define RTL931X_MAC_LINK_SPD_STS_500M                                   0x3
#define RTL931X_MAC_LINK_SPD_STS_1000M                                  0x2
#define RTL931X_MAC_LINK_SPD_STS_100M                                   0x1
#define RTL931X_MAC_LINK_SPD_STS_10M                                    0x0
#define RTL931X_MAC_LINK_SPD_STS(p, r) \
        (((r) >> (((p) % 8) * 4)) & _RTL931X_MAC_LINK_SPD_STS_MASK)

#define RTL931X_MAC_LINK_STS_REG(p)                     (0x0ec0 + (((p) / 32) * 0x4))
#define RTL931X_MAC_LINK_STS_MASK                               BIT(0)
#define RTL931X_MAC_LINK_STS_UP                                         0b1
#define RTL931X_MAC_LINK_STS_DOWN                                       0b0
#define RTL931X_MAC_LINK_STS(p, r) \
        (((r) >> ((p) % 32)) & RTL931X_MAC_LINK_STS_MASK)

#define RTL931X_MAC_RX_PAUSE_STS_REG(p)                 (0x0f00 + (((p) / 32) * 0x4))
#define _RTL931X_MAC_RX_PAUSE_STS_MASK                          BIT(0)
#define RTL931X_MAC_RX_PAUSE_STS_ON                                     0b1
#define RTL931X_MAC_RX_PAUSE_STS_OFF                                    0b0
#define RTL931X_MAC_RX_PAUSE_STS(p, r) \
        (((r) >> ((p) % 32)) & _RTL931X_MAC_RX_PAUSE_STS_MASK)

#define RTL931X_MAC_TX_PAUSE_STS_REG(p)                 (0x0ef8 + (((p) / 32) * 0x4))
#define _RTL931X_MAC_TX_PAUSE_STS_MASK                          BIT(0)
#define RTL931X_MAC_TX_PAUSE_STS_ON                                     0b1
#define RTL931X_MAC_TX_PAUSE_STS_OFF                                    0b0
#define RTL931X_MAC_TX_PAUSE_STS(p, r) \
        (((r) >> ((p) % 32)) & _RTL931X_MAC_TX_PAUSE_STS_MASK)

/* EEE */
#define RTL931X_EEE_TX_SEL_CTRL0_REG                    (0x5658)
#define RTL931X_EEE_TX_SEL_CTRL0_TX_LPI_MINIPG_500M             GENMASK(31, 16)
#define RTL931X_EEE_TX_SEL_CTRL0_TX_LPI_MINIPG_100M             GENMASK(15, 0)

#define RTL931X_EEE_TX_SEL_CTRL1_REG                    (0x565c)
#define RTL931X_EEE_TX_SEL_CTRL1_TX_LPI_MINIPG_10G              GENMASK(31, 16)
#define RTL931X_EEE_TX_SEL_CTRL1_TX_LPI_MINIPG_1000M            GENMASK(15, 0)

#define RTL931X_EEE_TX_SEL_CTRL2_REG                    (0x5660)
#define RTL931X_EEE_TX_SEL_CTRL2_TX_LPI_MINIPG_5G               GENMASK(31, 16)
#define RTL931X_EEE_TX_SEL_CTRL2_TX_LPI_MINIPG_2G5              GENMASK(15, 0)

#define RTL931X_EEE_PORT_TX_REG(p)                      (0x5644 + (((p) / 32) * 0x4))
#define _RTL931X_EEE_PORT_TX_EN_MASK                            BIT(0)
#define RTL931X_EEE_PORT_TX_EN_ON                               0b1
#define RTL931X_EEE_PORT_TX_EN_OFF                              0b0
#define RTL931X_EEE_PORT_TX(r, p) \
        (((r) >> ((p) % 32)) & _RTL931X_EEE_PORT_TX_EN_MASK)
#define RTL931X_EEE_PORT_TX_EN(p) \
        (((p) & _RTL931X_EEE_PORT_TX_EN_MASK) << ((p) % 32))

#define RTL931X_EEE_PORT_RX_REG(p)                      (0x564c + (((p) / 32) * 0x4))
#define _RTL831X_EEE_PORT_RX_EN_MASK                            BIT(0)
#define RTL931X_EEE_PORT_RX_EN_ON                               0b1
#define RTL931X_EEE_PORT_RX_EN_OFF                              0b0
#define RTL931X_EEE_PORT_RX(r, p) \
        (((r) >> ((p) % 32)) & _RTL831X_EEE_PORT_RX_EN_MASK)
#define RTL931X_EEE_PORT_RX_EN(p) \
        (((p) & _RTL831X_EEE_PORT_RX_EN_MASK) << ((p) % 32))

#define RTL931X_EEEP_PORT_TX_REG(p)                     (0x5698 + (((p) / 32) * 0x4))
#define _RTL931X_EEEP_PORT_TX_EN_MASK                           BIT(0)
#define RTL931X_EEEP_PORT_TX_EN_ON                              0b1
#define RTL931X_EEEP_PORT_TX_EN_OFF                             0b0
#define RTL931X_EEEP_PORT_TX(r, p) \
        (((r) >> ((p) % 32)) & _RTL931X_EEEP_PORT_TX_EN_MASK)
#define RTL931X_EEEP_PORT_TX_EN(p) \
        (((p) & _RTL931X_EEEP_PORT_TX_EN_MASK) << ((p) % 32))

#define RTL931X_EEEP_PORT_RX_REG(p)                     (0x56a0 + (((p) / 32) * 0x4))
#define _RTL831X_EEEP_PORT_RX_EN_MASK                           BIT(0)
#define RTL931X_EEEP_PORT_RX_EN_ON                              0b1
#define RTL931X_EEEP_PORT_RX_EN_OFF                             0b0
#define RTL931X_EEEP_PORT_RX(r, p) \
        (((r) >> ((p) % 32)) & _RTL831X_EEEP_PORT_RX_EN_MASK)
#define RTL931X_EEEP_PORT_RX_EN(p) \
        (((p) & _RTL831X_EEEP_PORT_RX_EN_MASK) << ((p) % 32))

#define RTL931X_EEE_TX_TIMER_100M_CTRL_REG              (0x5670)
/* Reserved                                                     31 - 20 */
#define RTL931X_EEE_TX_TIMER_100M_CTRL_TX_LOW_Q_DELAY           GENMASK(19, 8)
#define RTL931X_EEE_TX_TIMER_100M_CTRL_TX_WAKE                  GENMASK(7, 0)

#define RTL931X_EEE_TX_TIMER_500M_CTRL_REG              (0x5674)
/* Reserved                                                     31 - 28 */
#define RTL931X_EEE_TX_TIMER_500M_CTRL_TX_PAUSE_WAKE            GENMASK(27, 20)
#define RTL931X_EEE_TX_TIMER_500M_CTRL_TX_LOW_Q_DELAY           GENMASK(19, 8)
#define RTL931X_EEE_TX_TIMER_500M_CTRL_TX_WAKE                  GENMASK(7, 0)

#define RTL931X_EEE_TX_TIMER_1000M_CTRL_REG              (0x5678)
/* Reserved                                                     31 - 28 */
#define RTL931X_EEE_TX_TIMER_1000M_CTRL_TX_PAUSE_WAKE           GENMASK(27, 20)
#define RTL931X_EEE_TX_TIMER_1000M_CTRL_TX_LOW_Q_DELAY          GENMASK(19, 8)
#define RTL931X_EEE_TX_TIMER_1000M_CTRL_TX_WAKE                 GENMASK(7, 0)

#define RTL931X_EEE_TX_TIMER_2G5_CTRL_REG               (0x567c)
/* Reserved                                                     31 - 28 */
#define RTL931X_EEE_TX_TIMER_2G5_CTRL_TX_PAUSE_WAKE             GENMASK(27, 20)
#define RTL931X_EEE_TX_TIMER_2G5_CTRL_TX_LOW_Q_DELAY            GENMASK(19, 8)
#define RTL931X_EEE_TX_TIMER_2G5_CTRL_TX_WAKE                   GENMASK(7, 0)

#define RTL931X_EEE_TX_TIMER_5G_CTRL_REG                (0x5680)
/* Reserved                                                     31 - 28 */
#define RTL931X_EEE_TX_TIMER_5G_CTRL_TX_PAUSE_WAKE              GENMASK(27, 20)
#define RTL931X_EEE_TX_TIMER_5G_CTRL_TX_LOW_Q_DELAY             GENMASK(19, 8)
#define RTL931X_EEE_TX_TIMER_5G_CTRL_TX_WAKE                    GENMASK(7, 0)

#define RTL931X_EEE_TX_TIMER_10G_CTRL_REG               (0x5684)
/* Reserved                                                     31 - 28 */
#define RTL931X_EEE_TX_TIMER_10G_CTRL_TX_PAUSE_WAKE             GENMASK(27, 20)
#define RTL931X_EEE_TX_TIMER_10G_CTRL_TX_LOW_Q_DELAY            GENMASK(19, 8)
#define RTL931X_EEE_TX_TIMER_10G_CTRL_TX_WAKE                   GENMASK(7, 0)

/* L2 functionality */
#define	RTL931X_L2_PORT_AGE_CTRL		(0xc808)
#define RTL931X_L2_AGE_CTRL			(0xc804)
#define RTL931X_L2_BC_FLD_PMSK			(0xc8fc)
#define RTL931X_L2_CTRL				(0xc800)
#define RTL931X_L2_LRN_CONSTRT_CTRL		(0xc964)
#define RTL931X_L2_PORT_NEW_SALRN(p)		(0xc820 + (((p >> 4) << 2)))
#define RTL931X_L2_PORT_NEW_SA_FWD(p)		(0xc830 + (((p / 10) << 2)))
#define RTL931X_L2_TBL_FLUSH_CTRL		(0xcd9c)
#define RTL931X_L2_UNKN_UC_FLD_PMSK		(0xc8f4)

/* Port Mirroring */
#define RTL931X_MIR_CTRL			(0xaf00)
#define RTL931X_MIR_DPM_CTRL			(0xaf30)
#define RTL931X_MIR_SPM_CTRL			(0xaf10)

/* Link aggregation (Trunking) */
#define RTL931X_TRK_HASH_CTRL			(0xba70)
#define RTL931X_TRK_MBR_CTRL			(0xb8d0)
#define RTL931X_RMA_BPDU_CTRL			(0x881c)
#define RTL931X_RMA_PTP_CTRL			(0x8834)
#define RTL931X_RMA_LLTP_CTRL			(0x8918)
#define RTL931X_RMA_EAPOL_CTRL			(0x8930)
#define RTL931X_TRAP_ARP_GRAT_PORT_ACT		(0x8c04)

/* 802.1X */
#define RTL931X_RMA_BPDU_FLD_PMSK		(0x8950)

/* Packet Inspection Engine */
#define RTL931X_METER_GLB_CTRL			(0x411C)
#define RTL931X_PIE_BLK_LOOKUP_CTRL		(0x4180)
#define RTL931X_PIE_BLK_TMPLTE_CTRL(block)	(0x4214 + ((block) << 2))
#define RTL931X_PIE_CLR_CTRL			(0x42D8)
#define RTL931X_ACL_PORT_LOOKUP_CTRL(p)		(0x44F8 + (((p) << 2)))
#define RTL931X_PIE_BLK_PHASE_CTRL		(0x4184)

/* Port LED Control */
#define RTL931X_LED_PORT_NUM_CTRL(p)		(0x0604 + (((p >> 4) << 2)))
#define RTL931X_LED_SET0_0_CTRL			(0x0630)
#define RTL931X_LED_PORT_COPR_SET_SEL_CTRL(p)	(0x0634 + (((p >> 4) << 2)))
#define RTL931X_LED_PORT_FIB_SET_SEL_CTRL(p)	(0x0644 + (((p >> 4) << 2)))
#define RTL931X_LED_PORT_COPR_MASK_CTRL		(0x0654)
#define RTL931X_LED_PORT_FIB_MASK_CTRL		(0x065c)
#define RTL931X_LED_PORT_COMBO_MASK_CTRL	(0x0664)

/* Interrupt control */
/* IMR_GLB does not exit on RTL931X */

#define RTL931X_IMR_SERDES_ERR_REG                      (0x1274)
#define _RTL931X_IMR_SERDES_ERR_MASK                            BIT(0)
#define RTL931X_IMR_SERDES_ERR(p) \
        (_RTL931X_IMR_SERDES_ERR_MASK << (p))

#define RTL931X_IMR_SERDES_UPD_PHY_STS_REG(p)           (0x1290 + (((p) / 32) * 0x4))
#define _RTL931X_IMR_SERDES_UPD_PHY_STS_MASK                    BIT(0)
#define RTL931X_IMR_SERDES_UPD_PHY_STS(p) \
        (_RTL931X_IMR_SERDES_UPD_PHY_STS_MASK << ((p) % 32))

#define RTL931X_IMR_SERDES_RXIDLE_REG                   (0x12a0)
#define _RTL931X_IMR_SERDES_RXIDLE_MASK                         BIT(0)
#define RTL931X_IMR_SERDES_RXIDLE(p) \
        (_RTL931X_IMR_SERDES_RXIDLE_MASK << (p))

#define RTL931X_IMR_PORT_LINK_STS_REG(p)                (0x126c + (((p) / 32) * 0x4))
#define _RTL931X_IMR_PORT_LINK_STS_MASK                         BIT(0)
#define RTL931X_IMR_PORT_LINK_STS(p, m) \
        (((m) & _RTL931X_IMR_PORT_LINK_STS_MASK) << ((p) % 32))

#define RTL931X_ISR_GLB_SRC_REG                         (0x12b4)
/* Reserved                                                     31 - 17 */
#define RTL931X_ISR_GLB_SRC_DBGO                                BIT(16)
#define RTL931X_ISR_GLB_SRC_SERDES_RXIDLE                       BIT(15)
#define RTL931X_ISR_GLB_SRC_RLFD                                BIT(14)
#define RTL931X_ISR_GLB_SRC_STAT_TRIGGER                        BIT(13)
#define RTL931X_ISR_GLB_SRC_RMT_INTR_STS_UPD                    BIT(12)
#define RTL931X_ISR_GLB_SRC_AUTO_REC                            BIT(11)
#define RTL931X_ISR_GLB_SRC_TX_CRC_CNTR                         BIT(10)
#define RTL931X_ISR_GLB_SRC_SMI_CHEKC                           BIT(9)
#define RTL931X_ISR_GLB_SRC_SERDES_UPD_PHY_STS                  BIT(8)
#define RTL931X_ISR_GLB_SRC_TM                                  BIT(7)
#define RTL931X_ISR_GLB_SRC_EXT_GPIO                            BIT(6)
#define RTL931X_ISR_GLB_SRC_ETHDM                               BIT(5)
#define RTL931X_ISR_GLB_SRC_OAM_DYGASP                          BIT(4)
#define RTL931X_ISR_GLB_SRC_CCM                                 BIT(3)
#define RTL931X_ISR_GLB_SRC_TIMESTAMP_LATCH                     BIT(2)
#define RTL931X_ISR_GLB_SRC_SERDES                              BIT(1)
#define RTL931X_ISR_GLB_SRC_LINK_CHG                            BIT(0)

#define RTL931X_ISR_PORT_LINK_STS_REG(p)                (0x12b8 + ((p / 32) * 0x4))
#define _RTL931X_ISR_PORT_LINK_STS_MASK                         BIT(0)
#define RTL931X_ISR_PORT_LINK_STS(p, r) \
        (((r) >> ((p) % 32)) & _RTL931X_ISR_PORT_LINK_STS_MASK)

#define RTL931X_ISR_SERDES_ERR_REG                      (0x12c0)
#define _RTL931X_ISR_SERDES_ERR_MASK                            BIT(0)
#define RTL931X_ISR_SERDES_ERR(p, r) \
        (((r) << (p)) & _RTL931X_ISR_SERDES_ERR_MASK)
#define RTL931X_ISR_SERDES_ERR_CLR(p) \
        (_RTL931X_ISR_SERDES_ERR_MASK << (p))

#define RTL931X_ISR_SERDES_UPD_PHY_STS_REG(p)           (0x12e8 + ((p / 32) * 0x4))
#define _RTL931X_ISR_SERDES_UPD_PHY_STS_MASK                    BIT(0)
#define RTL931X_ISR_SERDES_UPD_PHY_STS(p, r) \
        (((r) << ((p) % 32)) & _RTL931X_ISR_SERDES_UPD_PHY_STS_MASK)
#define RTL931X_ISR_SERDES_UPD_PHY_STS_CLR(p) \
        (_RTL931X_ISR_SERDES_UPD_PHY_STS_MASK << ((p) % 32))

#define RTL931X_ISR_SERDES_RXIDLE_REG                   (0x12f8)
#define _RTL931X_ISR_SERDES_RXIDLE_MASK                         BIT(0)
#define RTL931X_ISR_SERDES_LINK_FAULT(p, r) \
        (((r) << (p)) & _RTL931X_ISR_SERDES_RXIDLE_MASK)
#define RTL931X_ISR_SERDES_LINK_FAULT_CLR(p) \
        (_RTL931X_ISR_SERDES_RXIDLE_MASK << (p))

void rtl931x_imr_port_link_sts_chg(const u64 ports);
void rtl931x_imr_serdes_upd_phy_sts(const u64 ports);
void rtl931x_isr_port_link_sts_chg(const u64 ports);
void rtl931x_isr_serdes_upd_phy_sts(const u64 ports);
int rtl931x_mac_force_mode_ctrl(const int port);
int rtl931x_mac_link_dup_sts(const int port);
int rtl931x_mac_link_media_sts(const int port);
int rtl931x_mac_link_spd_sts(const int port);
int rtl931x_mac_link_sts(const int port);
int rtl931x_mac_port_ctrl(const int port);
int rtl931x_mac_rx_pause_sts(const int port);
int rtl931x_mac_tx_pause_sts(const int port);
int rtl931x_sds_cmu_band_get(int sds, phy_interface_t mode);
int rtl931x_sds_cmu_band_set(int sds, bool enable, u32 band, phy_interface_t mode);
void rtl931x_sds_init(u32 sds, phy_interface_t mode);
irqreturn_t rtl931x_switch_irq(int irq, void *dev_id);

#endif /* _RTL931X_H */
