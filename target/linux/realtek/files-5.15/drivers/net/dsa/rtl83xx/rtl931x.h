/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _RTL931X_H
#define _RTL931X_H __FILE__

#include <linux/types.h>
#include <linux/phy.h>

/* MAC port control */
#define RTL931X_MAC_FORCE_MODE_CTRL		(0x0dcc)
#define RTL931X_MAC_L2_PORT_CTRL		(0x6000)
#define RTL931X_MAC_PORT_CTRL			(0x6004)
#define RTL931X_PORT_ISO_VB_ISO_PMSK_CTRL(port)	(0x8a04 + ((port) << 2))

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
#define RTL931X_MAC_LINK_DUP_STS_ADDR		(0x0ef0)
#define RTL931X_MAC_LINK_MEDIA_STS_ADDR		(0x0ec8)
#define RTL931X_MAC_LINK_SPD_STS_ADDR		(0x0ed0)
#define RTL931X_MAC_LINK_STS_ADDR		(0x0ec0)
#define RTL931X_MAC_RX_PAUSE_STS_ADDR		(0x0f00)
#define RTL931X_MAC_TX_PAUSE_STS_ADDR		(0x0ef8)

#define RTL931X_RX_PAUSE_EN			(1 << 17)
#define RTL931X_TX_PAUSE_EN			(1 << 16)
#define RTL931X_FORCE_EN			(1 << 9)
#define RTL931X_MAC_FORCE_FC_EN			(1 << 4)
#define RTL931X_DUPLEX_MODE			(1 << 2)
#define RTL931X_FORCE_LINK_EN			(1 << 0)

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

int rtl931x_sds_cmu_band_get(int sds, phy_interface_t mode);
int rtl931x_sds_cmu_band_set(int sds, bool enable, u32 band, phy_interface_t mode);
void rtl931x_sds_init(u32 sds, phy_interface_t mode);
irqreturn_t rtl931x_switch_irq(int irq, void *dev_id);

#endif /* _RTL931X_H */
