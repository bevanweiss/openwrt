/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _RTL838X_ETH_H
#define _RTL838X_ETH_H

/* Register definition */

#define RTL930X_L2_NTFY_IF_INTR_MSK		(0xe04C)
#define RTL930X_L2_NTFY_IF_INTR_STS		(0xe050)

#define RTL931X_L2_NTFY_IF_INTR_MSK		(0x09E4)
#define RTL931X_L2_NTFY_IF_INTR_STS		(0x09E8)

#define RTL838X_DMA_IF_RX_CUR			(0x9F20)
#define RTL839X_DMA_IF_RX_CUR			(0x782c)
#define RTL930X_DMA_IF_RX_CUR			(0xdf80)
#define RTL931X_DMA_IF_RX_CUR			(0x0880)

/* L2 features */
#define RTL839X_TBL_ACCESS_L2_CTRL		(0x1180)
#define RTL839X_TBL_ACCESS_L2_DATA(idx)		(0x1184 + ((idx) << 2))
#define RTL838X_TBL_ACCESS_CTRL_0		(0x6914)
#define RTL838X_TBL_ACCESS_DATA_0(idx)		(0x6918 + ((idx) << 2))

#define RTL930X_L2_PORT_SABLK_CTRL		(0x905c)
#define RTL930X_L2_PORT_DABLK_CTRL		(0x9060)

/* L2 Notification DMA interface */
#define RTL839X_DMA_IF_NBUF_BASE_DESC_ADDR_CTRL	(0x785C)
#define RTL839X_L2_NOTIFICATION_CTRL		(0x7808)
#define RTL931X_L2_NTFY_RING_BASE_ADDR		(0x09DC)
#define RTL931X_L2_NTFY_RING_CUR_ADDR		(0x09E0)
#define RTL839X_L2_NOTIFICATION_CTRL		(0x7808)
#define RTL931X_L2_NTFY_CTRL			(0xCDC8)
#define RTL838X_L2_CTRL_0			(0x3200)
#define RTL839X_L2_CTRL_0			(0x3800)

/* TRAPPING to CPU-PORT */
#define RTL838X_SPCL_TRAP_IGMP_CTRL		(0x6984)
#define RTL838X_RMA_CTRL_0			(0x4300)
#define RTL838X_RMA_CTRL_1			(0x4304)
#define RTL839X_RMA_CTRL_0			(0x1200)

#define RTL839X_SPCL_TRAP_IGMP_CTRL		(0x1058)
#define RTL839X_RMA_CTRL_1			(0x1204)
#define RTL839X_RMA_CTRL_2			(0x1208)
#define RTL839X_RMA_CTRL_3			(0x120C)

#define RTL930X_VLAN_APP_PKT_CTRL		(0xA23C)
#define RTL930X_RMA_CTRL_0			(0x9E60)
#define RTL930X_RMA_CTRL_1			(0x9E64)
#define RTL930X_RMA_CTRL_2			(0x9E68)

#define RTL931X_VLAN_APP_PKT_CTRL		(0x96b0)
#define RTL931X_RMA_CTRL_0			(0x8800)
#define RTL931X_RMA_CTRL_1			(0x8804)
#define RTL931X_RMA_CTRL_2			(0x8808)

/* Advanced SMI control for clause 45 PHYs */
#define RTL930X_SMI_MAC_TYPE_CTRL		(0xCA04)
#define RTL930X_SMI_PORT24_27_ADDR_CTRL		(0xCB90)
#define RTL930X_SMI_PORT0_15_POLLING_SEL	(0xCA08)
#define RTL930X_SMI_PORT16_27_POLLING_SEL	(0xCA0C)

#define RTL930X_SMI_10GPHY_POLLING_REG0_CFG	(0xCBB4)
#define RTL930X_SMI_10GPHY_POLLING_REG9_CFG	(0xCBB8)
#define RTL930X_SMI_10GPHY_POLLING_REG10_CFG	(0xCBBC)
#define RTL930X_SMI_PRVTE_POLLING_CTRL		(0xCA10)

/* Chip configuration registers of the RTL9310 */
#define RTL931X_MEM_ENCAP_INIT			(0x4854)
#define RTL931X_MEM_MIB_INIT			(0x7E18)
#define RTL931X_MEM_ACL_INIT			(0x40BC)
#define RTL931X_MEM_ALE_INIT_0			(0x83F0)
#define RTL931X_MEM_ALE_INIT_1			(0x83F4)
#define RTL931X_MEM_ALE_INIT_2			(0x82E4)
#define RTL931X_MDX_CTRL_RSVD			(0x0fcc)
#define RTL931X_PS_SOC_CTRL			(0x13f8)
#define RTL931X_SMI_10GPHY_POLLING_SEL2		(0xCF8)
#define RTL931X_SMI_10GPHY_POLLING_SEL3		(0xCFC)
#define RTL931X_SMI_10GPHY_POLLING_SEL4		(0xD00)

struct p_hdr;
struct dsa_tag;

struct rtl838x_eth_reg {
	irqreturn_t (*net_irq)(int irq, void *dev_id);
	int (*mac_port_ctrl)(const int port);
	int (*mac_force_mode_ctrl)(const int port);
	int dma_if_intr_sts;
	int dma_if_intr_msk;
	int dma_if_intr_rx_runout_sts;
	int dma_if_intr_rx_done_sts;
	int dma_if_intr_tx_done_sts;
	int dma_if_intr_rx_runout_msk;
	int dma_if_intr_rx_done_msk;
	int dma_if_intr_tx_done_msk;
	int l2_ntfy_if_intr_sts;
	int l2_ntfy_if_intr_msk;
	int dma_if_ctrl;
	int dma_rx_base;
	int dma_tx_base;
	int (*dma_if_rx_ring_size)(const int ring);
	int (*dma_if_rx_ring_cntr)(const int ring);
	int dma_if_rx_cur;
	int rst_glb_ctrl;
	int (*get_mac_link_sts)(const int port);
	int (*get_mac_link_dup_sts)(const int port);
	int (*get_mac_link_media_sts)(const int port);
	int (*get_mac_link_spd_sts)(const int port);
	int (*get_mac_rx_pause_sts)(const int port);
	int (*get_mac_tx_pause_sts)(const int port);
	int mac;
	int l2_tbl_flush_ctrl;
	void (*update_cntr)(int r, int work_done);
	void (*create_tx_header)(struct p_hdr *h, unsigned int dest_port, int prio);
	bool (*decode_tag)(struct p_hdr *h, struct dsa_tag *tag);
};

int rtl838x_write_phy(u32 port, u32 page, u32 reg, u32 val);
int rtl838x_read_phy(u32 port, u32 page, u32 reg, u32 *val);
int rtl838x_write_mmd_phy(u32 port, u32 addr, u32 reg, u32 val);
int rtl838x_read_mmd_phy(u32 port, u32 addr, u32 reg, u32 *val);
int rtl839x_write_phy(u32 port, u32 page, u32 reg, u32 val);
int rtl839x_read_phy(u32 port, u32 page, u32 reg, u32 *val);
int rtl839x_read_mmd_phy(u32 port, u32 devnum, u32 regnum, u32 *val);
int rtl839x_write_mmd_phy(u32 port, u32 devnum, u32 regnum, u32 val);
int rtl930x_write_phy(u32 port, u32 page, u32 reg, u32 val);
int rtl930x_read_phy(u32 port, u32 page, u32 reg, u32 *val);
int rtl931x_write_phy(u32 port, u32 page, u32 reg, u32 val);
int rtl931x_read_phy(u32 port, u32 page, u32 reg, u32 *val);
int rtl83xx_setup_tc(struct net_device *dev, enum tc_setup_type type, void *type_data);

#endif /* _RTL838X_ETH_H */
