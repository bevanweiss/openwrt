/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _RTL839X_H
#define _RTL839X_H __FILE__

#include <linux/types.h>
#include <net/dsa.h>

#include "rtl83xx.h"

#define RTL8390_VERSION_A 'A'

/* MAC port control */
#define RTL839X_MAC_FORCE_MODE_CTRL		(0x02bc)
#define RTL839X_MAC_PORT_CTRL(port)		(0x8004 + (((port) << 7)))
#define RTL839X_PORT_ISO_CTRL(port)		(0x1400 + ((port) << 3))

/* Packet statistics */
#define RTL839X_STAT_CTRL			(0x04cc)
#define RTL839X_STAT_PORT_RST			(0xf508)
#define RTL839X_STAT_PORT_STD_MIB		(0xc000)
#define RTL839X_STAT_RST			(0xf504)

/* Registers of the internal Serdes */
#define RTL8390_SDS0_1_XSG0			(0xa000)
#define RTL8390_SDS0_1_XSG1			(0xa100)
#define RTL839X_SDS12_13_PWR0			(0xb880)
#define RTL839X_SDS12_13_PWR1			(0xb980)
#define RTL839X_SDS12_13_XSG0			(0xb800)
#define RTL839X_SDS12_13_XSG1			(0xb900)

/* VLAN registers */
#define RTL839X_VLAN_CTRL			(0x26d4)
#define RTL839X_VLAN_PORT_EGR_FLTR		(0x27c4)
#define RTL839X_VLAN_PORT_FWD			(0x27ac)
#define RTL839X_VLAN_PORT_IGR_FLTR		(0x27b4)
#define RTL839X_VLAN_PORT_PB_VLAN		(0x26d8)
#define RTL839X_VLAN_PROFILE(idx)		(0x25c0 + (((idx) << 3)))

/* Table access registers */
#define RTL839X_TBL_ACCESS_CTRL_0		(0x1190)
#define RTL839X_TBL_ACCESS_DATA_0(idx)		(0x1194 + ((idx) << 2))
#define RTL839X_TBL_ACCESS_CTRL_1		(0x6b80)
#define RTL839X_TBL_ACCESS_DATA_1(idx)		(0x6b84 + ((idx) << 2))
#define RTL839X_TBL_ACCESS_CTRL_2		(0x611c)
#define RTL839X_TBL_ACCESS_DATA_2(i)		(0x6120 + (((i) << 2)))

/* MAC handling */
#define RTL839X_MAC_LINK_DUP_STS		(0x03b0)
#define RTL839X_MAC_LINK_SPD_STS(p)		(0x03a0 + (((p >> 4) << 2)))
#define RTL839X_MAC_LINK_STS			(0x0390)
#define RTL839X_MAC_RX_PAUSE_STS		(0x03c0)
#define RTL839X_MAC_TX_PAUSE_STS		(0x03b8)

#define RTL839X_FORCE_EN			(1 << 0)
#define RTL839X_FORCE_LINK_EN			(1 << 1)
#define RTL839X_DUPLEX_MODE			(1 << 2)
#define RTL839X_TX_PAUSE_EN			(1 << 5)
#define RTL839X_RX_PAUSE_EN			(1 << 6)
#define RTL839X_MAC_FORCE_FC_EN			(1 << 7)

/* EEE */
#define RTL839X_EEE_CTRL(p)			(0x8008 + ((p) << 7))
#define RTL839X_EEE_TX_TIMER_10G_CTRL		(0x0434)
#define RTL839X_EEE_TX_TIMER_GELITE_CTRL	(0x042c)
#define RTL839X_EEE_TX_TIMER_GIGA_CTRL		(0x0430)
#define RTL839X_MAC_EEE_ABLTY			(0x03c8)

/* L2 functionality */
#define RTL839X_L2_CTRL_0			(0x3800)
#define RTL839X_L2_CTRL_1			(0x3804)
#define RTL839X_L2_FLD_PMSK			(0x38ec)
#define RTL839X_L2_LRN_CONSTRT			(0x3910)
#define RTL839X_L2_PORT_AGING_OUT		(0x3b74)
#define RTL839X_L2_PORT_LRN_CONSTRT		(0x3914)
#define RTL839X_L2_PORT_NEW_SALRN(p)		(0x38f0 + (((p >> 4) << 2)))
#define RTL839X_L2_PORT_NEW_SA_FWD(p)		(0x3900 + (((p >> 4) << 2)))
#define RTL839X_L2_TBL_FLUSH_CTRL		(0x3ba0)
#define RTL839X_TBL_ACCESS_L2_CTRL		(0x1180)
#define RTL839X_TBL_ACCESS_L2_DATA(idx)		(0x1184 + ((idx) << 2))

/* Port Mirroring */
#define RTL839X_MIR_CTRL			(0x2500)
#define RTL839X_MIR_DPM_CTRL			(0x2530)
#define RTL839X_MIR_SPM_CTRL			(0x2510)

/* Storm/rate control and scheduling */
#define RTL839X_IGR_BWCTRL_CTRL_LB_THR		(0x1614)
#define RTL839X_IGR_BWCTRL_LB_TICK_TKN_CTRL_0	(0x1604)
#define RTL839X_IGR_BWCTRL_LB_TICK_TKN_CTRL_1	(0x1608)
#define RTL839X_IGR_BWCTRL_PORT_CTRL_0(p)	(0x1640 + (((p) << 3)))
#define RTL839X_IGR_BWCTRL_PORT_CTRL_1(p)	(0x1644 + (((p) << 3)))
#define RTL839X_IGR_BWCTRL_PORT_CTRL_10G_0(p)	(0x1618 + (((p) << 3)))
#define RTL839X_IGR_BWCTRL_PORT_CTRL_10G_1(p)	(0x161c + (((p) << 3)))
#define RTL839X_SCHED_CTRL			(0x60f4)
#define RTL839X_SCHED_LB_THR			(0x60fc)
#define RTL839X_SCHED_LB_TICK_TKN_CTRL		(0x60f8)
#define RTL839X_SCHED_LB_TICK_TKN_CTRL_0	(0x1804)
#define RTL839X_SCHED_LB_TICK_TKN_CTRL_1	(0x1808)
#define RTL839X_SCHED_LB_TICK_TKN_PPS_CTRL	(0x6200)
#define RTL839X_STORM_CTRL			(0x1800)
#define RTL839X_STORM_CTRL_LB_TICK_TKN_CTRL_0	(0x1804)
#define RTL839X_STORM_CTRL_LB_TICK_TKN_CTRL_1	(0x1808)
#define RTL839X_STORM_CTRL_PORT_BC_0(p)		(0x1b9c + (((p) << 3)))
#define RTL839X_STORM_CTRL_PORT_BC_1(p)		(0x1ba0 + (((p) << 3)))
#define RTL839X_STORM_CTRL_PORT_BC_EXCEED(p)	(0x180c + (((p >> 5) << 2)))
#define RTL839X_STORM_CTRL_PORT_MC_0(p)		(0x19fc + (((p) << 3)))
#define RTL839X_STORM_CTRL_PORT_MC_1(p)		(0x1a00 + (((p) << 3)))
#define RTL839X_STORM_CTRL_PORT_MC_EXCEED(p)	(0x1814 + (((p >> 5) << 2)))
#define RTL839X_STORM_CTRL_PORT_UC_0(p)		(0x185c + (((p) << 3)))
#define RTL839X_STORM_CTRL_PORT_UC_1(p)		(0x1860 + (((p) << 3)))
#define RTL839X_STORM_CTRL_PORT_UC_EXCEED(p)	(0x181c + (((p >> 5) << 2)))
#define RTL839X_STORM_CTRL_SPCL_LB_TICK_TKN_CTRL (0x2000)

/* Link aggregation (Trunking) */
#define RTL839X_TRK_HASH_CTRL	 		(0x2284)
#define RTL839X_TRK_HASH_IDX_CTRL		(0x2280)
#define RTL839X_TRK_MBR_CTR			(0x2200)

/* 802.1X */
#define RTL839X_RMA_BPDU_CTRL			(0x122c)
#define RTL839X_RMA_BPDU_FLD_PMSK		(0x125c)
#define RTL839X_RMA_LLTP_CTRL			(0x124c)
#define RTL839X_RMA_PTP_CTRL			(0x123c)
#define RTL839X_SPCL_TRAP_ARP_CTRL		(0x1060)
#define RTL839X_SPCL_TRAP_CRC_CTRL		(0x1070)
#define RTL839X_SPCL_TRAP_CTRL			(0x1054)
#define RTL839X_SPCL_TRAP_EAPOL_CTRL		(0x105c)
#define RTL839X_SPCL_TRAP_IGMP_CTRL		(0x1058)
#define RTL839X_SPCL_TRAP_IPV6_CTRL		(0x1064)
#define RTL839X_SPCL_TRAP_SWITCH_IPV4_ADDR_CTRL	(0x106c)
#define RTL839X_SPCL_TRAP_SWITCH_MAC_CTRL	(0x1068)

/* QoS */
#define RTL839X_OAM_CTRL			(0x2100)
#define RTL839X_OAM_PORT_ACT_CTRL(p)		(0x2104 + (((p) << 2)))
#define RTL839X_PRI_SEL_CTRL			(0x10e0)
#define RTL839X_PRI_SEL_DEI2DP_REMAP		(0x10ec)
#define RTL839X_PRI_SEL_DSCP2DP_REMAP_ADDR(i)	(0x10f0 + (((i >> 4) << 2)))
#define RTL839X_PRI_SEL_IPRI_REMAP		(0x1080)
#define RTL839X_PRI_SEL_PORT_PRI(p)		(0x10a8 + (((p / 10) << 2)))
#define RTL839X_PRI_SEL_TBL_CTRL(i)		(0x10d0 + (((i) << 2)))
#define RTL839X_QM_INTPRI2QID_CTRL(q)		(0x1110 + (q << 2))
#define RTL839X_QM_PKT2CPU_INTPRI_MAP		(0x1154)
#define RTL839X_QM_PORT_QNUM(p)			(0x1130 + (((p / 10) << 2)))
#define RTL839X_RMK_DEI_CTRL			(0x6aa4)
#define RTL839X_RMK_PORT_DEI_TAG_CTRL(p)	(0x6a9c + (((p >> 5) << 2)))
#define RTL839X_WRED_PORT_THR_CTRL(i)		(0x6084 + ((i) << 2))
#define RTL839X_WRED_QUEUE_THR_CTRL(q, i)	(0x6090 + ((q) * 12) + ((i) << 2))

/* Packet Inspection Engine */
#define RTL839X_ACL_BLK_GROUP_CTRL		(0x12ec)
#define RTL839X_ACL_BLK_LOOKUP_CTRL		(0x1280)
#define RTL839X_ACL_BLK_TMPLTE_CTRL(block)	(0x128c + ((block) << 2))
#define RTL839X_ACL_CLR_CTRL			(0x12fc)
#define RTL839X_ACL_CTRL			(0x1288)
#define RTL839X_METER_GLB_CTRL			(0x1300)
#define RTL839X_PS_ACL_PWR_CTRL			(0x049c)

/* L3 Routing */
#define RTL839X_ROUTING_SA_CTRL			(0x6afc)

/* MISC Register definitions */
#define RTL839X_CHIP_INFO			(0x0ff4)

void rtl839x_exec_tbl2_cmd(u32 cmd);
void rtl8390_get_version(struct rtl838x_switch_priv *priv);
u32 rtl839x_hash(struct rtl838x_switch_priv *priv, u64 seed);
void rtl839x_print_matrix(void);
irqreturn_t rtl839x_switch_irq(int irq, void *dev_id);
void rtl839x_vlan_profile_dump(int index);

#endif /* _RTL839X_H */
