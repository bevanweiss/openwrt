/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2006-2012 Tony Wu (tonywu@realtek.com)
 */
#ifndef _MACH_REALTEK_OTTO_H_
#define _MACH_REALTEK_OTTO_H_

#include <asm/types.h>
#include <linux/bitops.h>

/* The size of a register for the otto platform */
#define REALTEK_REG_SIZE        BITS_PER_TYPE(u32)

/* Register port offset. e.g. 16 config bits per port -> 2 ports per reg */
#define REALTEK_REG_PORT_OFFSET(port, bits_per_port, offset) \
        (((port) / (REALTEK_REG_SIZE / (bits_per_port))) * offset)

/* Register port index. e.g. 3rd port is the first set of bits of the reg */
#define REALTEK_REG_PORT_INDEX(port, bits_per_port) \
        (((port) % (REALTEK_REG_SIZE / (bits_per_port))) * (bits_per_port))

/* Size of array to hold all port config bits. E.g. 4 bits per port needs 2 elements (if port num < 32) */
#define REALTEK_PORT_ARRAY_SIZE(port, bits_per_port) \
        ((((port) / REALTEK_REG_SIZE) + 1) * bits_per_port)

/* Index to element in array holding config bits. E.g. port 9 with 4 bits per port, yields index 1 */
#define REALTEK_PORT_ARRAY_INDEX(port, bits_per_port) \
        (((port) * (bits_per_port)) / REALTEK_REG_SIZE)

/* Get (first) port matching an index. E.g. with 2 bits per port and array index 1 -> port 16 */
#define REALTEK_INDEX_ARRAY_PORT(index, bits_per_port) \
        ((index) * (REALTEK_REG_SIZE / (bits_per_port)))

/*
 * Register access macros
 */

#define RTL838X_SW_BASE		((volatile void *) 0xBB000000)

#define rtl83xx_r32(reg)	readl(reg)
#define rtl83xx_w32(val, reg)	writel(val, reg)
#define rtl83xx_w32_mask(clear, set, reg) rtl83xx_w32((rtl83xx_r32(reg) & ~(clear)) | (set), reg)

#define rtl83xx_r8(reg)		readb(reg)
#define rtl83xx_w8(val, reg)	writeb(val, reg)

#define sw_r32(reg)		readl(RTL838X_SW_BASE + reg)
#define sw_w32(val, reg)	writel(val, RTL838X_SW_BASE + reg)
#define sw_w32_mask(clear, set, reg)	\
				sw_w32((sw_r32(reg) & ~(clear)) | (set), reg)
#define sw_r64(reg)		((((u64)readl(RTL838X_SW_BASE + reg)) << 32) | \
				readl(RTL838X_SW_BASE + reg + 4))

#define sw_w64(val, reg)	do { \
					writel((u32)((val) >> 32), RTL838X_SW_BASE + reg); \
					writel((u32)((val) & 0xffffffff), \
							RTL838X_SW_BASE + reg + 4); \
				} while (0)

/*
 * SPRAM
 */
#define RTL838X_ISPRAM_BASE	0x0
#define RTL838X_DSPRAM_BASE	0x0

/*
 * IRQ Controller
 */
#define RTL838X_IRQ_CPU_BASE	0
#define RTL838X_IRQ_CPU_NUM	8
#define RTL838X_IRQ_ICTL_BASE	(RTL838X_IRQ_CPU_BASE + RTL838X_IRQ_CPU_NUM)
#define RTL838X_IRQ_ICTL_NUM	32

#define RTL83XX_IRQ_UART0		31
#define RTL83XX_IRQ_UART1		30
#define RTL83XX_IRQ_TC0			29
#define RTL83XX_IRQ_TC1			28
#define RTL83XX_IRQ_OCPTO		27
#define RTL83XX_IRQ_HLXTO		26
#define RTL83XX_IRQ_SLXTO		25
#define RTL83XX_IRQ_NIC			24
#define RTL83XX_IRQ_GPIO_ABCD		23
#define RTL83XX_IRQ_GPIO_EFGH		22
#define RTL83XX_IRQ_RTC			21
#define RTL83XX_IRQ_SWCORE		20
#define RTL83XX_IRQ_WDT_IP1		19
#define RTL83XX_IRQ_WDT_IP2		18

#define RTL9300_UART1_IRQ		31
#define RTL9300_UART0_IRQ		30
#define RTL9300_USB_H2_IRQ		28
#define RTL9300_NIC_IRQ			24
#define RTL9300_SWCORE_IRQ		23
#define RTL9300_GPIO_ABC_IRQ		13
#define RTL9300_TC4_IRQ			11
#define RTL9300_TC3_IRQ			10
#define RTL9300_TC2_IRQ			 9
#define RTL9300_TC1_IRQ			 8
#define RTL9300_TC0_IRQ			 7


/*
 * MIPS32R2 counter
 */
#define RTL838X_COMPARE_IRQ	(RTL838X_IRQ_CPU_BASE + 7)

/*
 *  ICTL
 *  Base address 0xb8003000UL
 */
#define RTL838X_ICTL1_IRQ	(RTL838X_IRQ_CPU_BASE + 2)
#define RTL838X_ICTL2_IRQ	(RTL838X_IRQ_CPU_BASE + 3)
#define RTL838X_ICTL3_IRQ	(RTL838X_IRQ_CPU_BASE + 4)
#define RTL838X_ICTL4_IRQ	(RTL838X_IRQ_CPU_BASE + 5)
#define RTL838X_ICTL5_IRQ	(RTL838X_IRQ_CPU_BASE + 6)

#define GIMR			(0x00)
#define UART0_IE		(1 << 31)
#define UART1_IE		(1 << 30)
#define TC0_IE			(1 << 29)
#define TC1_IE			(1 << 28)
#define OCPTO_IE		(1 << 27)
#define HLXTO_IE		(1 << 26)
#define SLXTO_IE		(1 << 25)
#define NIC_IE			(1 << 24)
#define GPIO_ABCD_IE		(1 << 23)
#define GPIO_EFGH_IE		(1 << 22)
#define RTC_IE			(1 << 21)
#define WDT_IP1_IE		(1 << 19)
#define WDT_IP2_IE		(1 << 18)

#define GISR			(0x04)
#define UART0_IP		(1 << 31)
#define UART1_IP		(1 << 30)
#define TC0_IP			(1 << 29)
#define TC1_IP			(1 << 28)
#define OCPTO_IP		(1 << 27)
#define HLXTO_IP		(1 << 26)
#define SLXTO_IP		(1 << 25)
#define NIC_IP			(1 << 24)
#define GPIO_ABCD_IP		(1 << 23)
#define GPIO_EFGH_IP		(1 << 22)
#define RTC_IP			(1 << 21)
#define WDT_IP1_IP		(1 << 19)
#define WDT_IP2_IP		(1 << 18)


/* Interrupt Routing Selection */
#define UART0_RS		2
#define UART1_RS		1
#define TC0_RS			5
#define TC1_RS			1
#define OCPTO_RS		1
#define HLXTO_RS		1
#define SLXTO_RS		1
#define NIC_RS			4
#define GPIO_ABCD_RS		4
#define GPIO_EFGH_RS		4
#define RTC_RS			4
#define	SWCORE_RS		3
#define WDT_IP1_RS		4
#define WDT_IP2_RS		5

/* Interrupt IRQ Assignments */
#define UART0_IRQ		31
#define UART1_IRQ		30
#define TC0_IRQ			29
#define TC1_IRQ			28
#define OCPTO_IRQ		27
#define HLXTO_IRQ		26
#define SLXTO_IRQ		25
#define NIC_IRQ			24
#define GPIO_ABCD_IRQ		23
#define GPIO_EFGH_IRQ		22
#define RTC_IRQ			21
#define	SWCORE_IRQ		20
#define WDT_IP1_IRQ		19
#define WDT_IP2_IRQ		18

#define SYSTEM_FREQ		200000000

/*
 * Memory Controller
 */
#define MC_MCR			0xB8001000
#define MC_MCR_VAL		0x00000000

#define MC_DCR			0xB8001004
#define MC_DCR0_VAL		0x54480000

#define MC_DTCR			0xB8001008
#define MC_DTCR_VAL		0xFFFF05C0

/*
 * GPIO
 */
#define RTL838X_MODEL_NAME_INFO		(0x00D4)
#define RTL839X_MODEL_NAME_INFO		(0x0FF0)
#define RTL93XX_MODEL_NAME_INFO		(0x0004)
#define RTL931X_CHIP_INFO_ADDR		(0x0008)

#define RTL838X_DMY_REG5		(0x0144)

#define RTL838X_GMII_INTF_SEL		(0x1000)
#define RTL838X_IO_DRIVING_ABILITY_CTRL	(0x1010)

#define RTL838X_GPIO_A7		31
#define RTL838X_GPIO_A6		30
#define RTL838X_GPIO_A5		29
#define RTL838X_GPIO_A4		28
#define RTL838X_GPIO_A3		27
#define RTL838X_GPIO_A2		26
#define RTL838X_GPIO_A1		25
#define RTL838X_GPIO_A0		24
#define RTL838X_GPIO_B7		23
#define RTL838X_GPIO_B6		22
#define RTL838X_GPIO_B5		21
#define RTL838X_GPIO_B4		20
#define RTL838X_GPIO_B3		19
#define RTL838X_GPIO_B2		18
#define RTL838X_GPIO_B1		17
#define RTL838X_GPIO_B0		16
#define RTL838X_GPIO_C7		15
#define RTL838X_GPIO_C6		14
#define RTL838X_GPIO_C5		13
#define RTL838X_GPIO_C4		12
#define RTL838X_GPIO_C3		11
#define RTL838X_GPIO_C2		10
#define RTL838X_GPIO_C1		9
#define RTL838X_GPIO_C0		8

#define RTL838X_INT_RW_CTRL		(0x0058)
#define RTL838X_EXT_VERSION		(0x00D0)
#define RTL838X_PLL_CML_CTRL		(0x0FF8)
#define RTL838X_STRAP_DBG		(0x100C)

/*
 * Reset
 */
#define RTL838X_RST_GLB_CTRL_0                                  (0x003c)
/* Reserved                                                             31 - 17 */
#define RTL838X_RST_GLB_CTRL_0_TBL_EGVLAN_RST                           BIT(16)
#define RTL838X_RST_GLB_CTRL_0_TBL_UNTAG_RST                            BIT(15)
#define RTL838X_RST_GLB_CTRL_0_TBL_MSTI_RST                             BIT(14)
#define RTL838X_RST_GLB_CTRL_0_TBL_ROUTING_RST                          BIT(13)
#define RTL838X_RST_GLB_CTRL_0_TBL_LOG_RST                              BIT(12)
#define RTL838X_RST_GLB_CTRL_0_TBL_ACL_RST                              BIT(11)
#define RTL838X_RST_GLB_CTRL_0_TBL_VLAN_RST                             BIT(10)
#define RTL838X_RST_GLB_CTRL_0_TBL_MC_PMSK_RST                          BIT(9)
#define RTL838X_RST_GLB_CTRL_0_TBL_L2_RST                               BIT(8)
/* Reserved                                                             7 */
#define RTL838X_RST_GLB_CTRL_0_CPU_MEM_RST                              BIT(6)
#define RTL838X_RST_GLB_CTRL_0_SW_PHY_RST                               BIT(5)
#define RTL838X_RST_GLB_CTRL_0_SW_SERDES_RST                            BIT(4)
#define RTL838X_RST_GLB_CTRL_0_SW_NIC_RST                               BIT(3)
#define RTL838X_RST_GLB_CTRL_0_SW_Q_RST                                 BIT(2)
#define RTL838X_RST_GLB_CTRL_0_SW_CFG_RST                               BIT(1)
#define RTL838X_RST_GLB_CTRL_0_SW_RST                                   BIT(0)

#define RTL838X_RST_GLB_CTRL_1                                  (0x0040)
/* Reserved                                                             31 - 1 */
#define RTL838X_RST_GLB_CTRL_1_HW_RST                                   BIT(0)

#define RTL839X_RST_GLB_CTRL                                    (0x014)
/* Reserved                                                             31 - 20 */
#define RTL839X_RST_GLB_CTRL_SYS_READY                                  BIT(19)
#define RTL839X_RST_GLB_CTRL_PKT_BUF_PROTECT_FLG                        BIT(18)
#define RTL839X_RST_GLB_CTRL_PKT_BUF_PROTECT_EN                         BIT(17)
#define RTL839X_RST_GLB_CTRL_TBL_UNTAG_RST                              BIT(16)
#define RTL839X_RST_GLB_CTRL_TBL_MSTI_RST                               BIT(15)
#define RTL839X_RST_GLB_CTRL_TBL_ROUTING_RST                            BIT(14)
#define RTL839X_RST_GLB_CTRL_TBL_LOG_RST                                BIT(13)
#define RTL839X_RST_GLB_CTRL_TBL_METER_RST                              BIT(12)
#define RTL839X_RST_GLB_CTRL_TBL_ACL_RST                                BIT(11)
#define RTL839X_RST_GLB_CTRL_TBL_VLAN_EGR_CNVT_RST                      BIT(10)
#define RTL839X_RST_GLB_CTRL_TBL_VLAN_IGR_CNVT_RST                      BIT(9)
#define RTL839X_RST_GLB_CTRL_TBL_VLAN_RST                               BIT(8)
#define RTL839X_RST_GLB_CTRL_TBL_MC_PMSK_RST                            BIT(7)
#define RTL839X_RST_GLB_CTRL_TBL_L2_RST                                 BIT(6)
#define RTL839X_RST_GLB_CTRL_CPU_MEM_RST                                BIT(5)
#define RTL839X_RST_GLB_CTRL_SW_SERDES_RST                              BIT(4)
#define RTL839X_RST_GLB_CTRL_SW_NIC_RST                                 BIT(3)
#define RTL839X_RST_GLB_CTRL_SW_Q_RST                                   BIT(2)
#define RTL839X_RST_GLB_CTRL_SW_CFG_RST                                 BIT(1)
#define RTL839X_RST_GLB_CTRL_SW_RST                                     BIT(0)

#define RTL930X_RST_GLB_CTRL_0                                  (0x00c)
/* Reserved                                                             31 - 12 */
#define RTL930X_RST_GLB_CTRL_0_RST_OUT_TMR                              BIT(11)
#define RTL930X_RST_GLB_CTRL_0_FRC_RSTOUT                               BIT(10)
#define RTL930X_RST_GLB_CTRL_0_FRC_RSTOUT_EN                            BIT(9)
#define RTL930X_RST_GLB_CTRL_0_CHIP_RST_EN                              BIT(8)
#define RTL930X_RST_GLB_CTRL_0_WD_RST_EN                                BIT(7)
#define RTL930X_RST_GLB_CTRL_0_SDS_REG_RST                              BIT(6)
#define RTL930X_RST_GLB_CTRL_0_SW_RST                                   BIT(5)
#define RTL930X_RST_GLB_CTRL_0_CPU_MEM_RST                              BIT(4)
#define RTL930X_RST_GLB_CTRL_0_SW_SERDES_RST                            BIT(3)
#define RTL930X_RST_GLB_CTRL_0_SW_NIC_RST                               BIT(2)
#define RTL930X_RST_GLB_CTRL_0_SW_Q_RST                                 BIT(1)
#define RTL930X_RST_GLB_CTRL_0_SW_CHIP_RST                              BIT(0)

#define RTL931X_RST_GLB_CTRL                                    (0x400)
/* Reserved                                                             31 - 20 */
#define RTL931X_RST_GLB_CTRL_BYP_NIC_ACK_TIMEOUT                        BIT(19)
#define RTL931X_RST_GLB_CTRL_REG_POR_PULL_CTRL                          GENMASK(18, 17)
#define RTL931X_RST_GLB_CTRL_REG_POR_LVDTCT                             BIT(16)
#define RTL931X_RST_GLB_CTRL_RST_PULSE_WIDTH                            GENMASK(15, 8)
#define RTL931X_RST_GLB_CTRL_MDX_GLB_RST                                BIT(7)
#define RTL931X_RST_GLB_CTRL_CHIP_RST_EN                                BIT(6)
#define RTL931X_RST_GLB_CTRL_WDOG_RST_EN                                BIT(5)
#define RTL931X_RST_GLB_CTRL_CPU_MEM_RST                                BIT(4)
#define RTL931X_RST_GLB_CTRL_SW_SERDES_RST                              BIT(3)
#define RTL931X_RST_GLB_CTRL_SW_NIC_RST                                 BIT(2)
#define RTL931X_RST_GLB_CTRL_SW_Q_RST                                   BIT(1)
#define RTL931X_RST_GLB_CTRL_SW_CHIP_RST                                BIT(0)

#define	RGCR				(0x1E70)

/* LED control by switch */
#define RTL838X_LED_MODE_SEL		(0x1004)
#define RTL838X_LED_MODE_CTRL		(0xA004)
#define RTL838X_LED_P_EN_CTRL		(0xA008)

/* LED control by software */
#define RTL838X_LED_SW_CTRL		(0x0128)
#define RTL839X_LED_SW_CTRL		(0xA00C)
#define RTL838X_LED_SW_P_EN_CTRL	(0xA010)
#define RTL839X_LED_SW_P_EN_CTRL	(0x012C)
#define RTL838X_LED0_SW_P_EN_CTRL	(0xA010)
#define RTL839X_LED0_SW_P_EN_CTRL	(0x012C)
#define RTL838X_LED1_SW_P_EN_CTRL	(0xA014)
#define RTL839X_LED1_SW_P_EN_CTRL	(0x0130)
#define RTL838X_LED2_SW_P_EN_CTRL	(0xA018)
#define RTL839X_LED2_SW_P_EN_CTRL	(0x0134)
#define RTL838X_LED_SW_P_CTRL		(0xA01C)
#define RTL839X_LED_SW_P_CTRL		(0x0144)

#define RTL839X_MAC_EFUSE_CTRL		(0x02ac)

/*
 * MDIO via Realtek's SMI interface
 */
#define RTL838X_SMI_GLB_CTRL		(0xa100)
#define RTL838X_SMI_ACCESS_PHY_CTRL_0	(0xa1b8)
#define RTL838X_SMI_ACCESS_PHY_CTRL_1	(0xa1bc)
#define RTL838X_SMI_ACCESS_PHY_CTRL_2	(0xa1c0)
#define RTL838X_SMI_ACCESS_PHY_CTRL_3	(0xa1c4)

#define RTL838X_SMI_PHY_PORT_CTRL_REG(p)                (0xa1b8 + (((p) / 32) * 0x4))
/* Reserved                                                     31 - 29 */
#define RTL838X_SMI_PHY_PORT_CTRL_PHY_MASK                      GENMASK(28, 0)
#define RTL838X_SMI_PHY_PORT_CTRL_PHY(p) \
        (BIT((p) % 32) & RTL838X_SMI_PHY_PORT_CTRL_PHY_MASK)

#define RTL839X_SMI_PHY_PORT_CTRL_REG(p)                (0x03e4 + (((p) / 32) * 0x4))
#define RTL839X_SMI_PHY_PORT_CTRL_PHY_MASK                      GENMASK(31, 0)
#define RTL839X_SMI_PHY_PORT_CTRL_PHY(p) \
        (BIT((p) % 32) & RTL839X_SMI_PHY_PORT_CTRL_PHY_MASK)

#define RTL930X_SMI_PHY_PORT_CTRL_REG(p)                (0xcb70 + (((p) / 32) * 0x4))
/* Reserved                                                     31 - 29 */
/* When broadcast bit is set, PHY_MASK mask sets broadcast address */
#define RTL930X_SMI_PHY_PORT_CTRL_PHY_BROADCAST                 BIT(28)
#define RTL930X_SMI_PHY_PORT_CTRL_PHY_BROADCAST_ADDR            GENMASK(4, 0)
#define RTL930X_SMI_PHY_PORT_CTRL_PHY_MASK                      GENMASK(27, 0)
#define RTL930X_SMI_PHY_PORT_CTRL_PHY(p) \
        (BIT((p) % 32) & RTL930X_SMI_PHY_PORT_CTRL_PHY_MASK)

#define RTL931X_SMI_PHY_PORT_CTRL_REG(p)                (0x0c08 + (((p) / 32) * 0x4))
#define RTL931X_SMI_PHY_PORT_CTRL_PHY_MASK                      GENMASK(31, 0)
#define RTL931X_SMI_PHY_PORT_CTRL_PHY(p) \
        (BIT((p) % 32) & RTL931X_SMI_PHY_PORT_CTRL_PHY_MASK)

#define RTL839X_SMI_PHY_CTRL_REG                        (0x03e0)
/* Reserved                                                     31 - 9 */
#define RTL839X_SMI_PHY_CTRL_EXT_PAGE                           GENMASK(8, 0)

#define RTL931X_SMI_PHY_CTRL_REG                        (0x0c04)
/* Reserved                                                     31 - 9 */
#define RTL931X_SMI_PHY_CTRL_EXT_PAGE                           GENMASK(8, 0)

#define RTL838X_SMI_ACCESS_PHY_CTRL_REG                 (0xa1bc)
/* Reserved                                                     31 - 25 */
#define RTL838X_SMI_ACCESS_PHY_CTRL_REG_ADDR                    GENMASK(24, 20)
#define RTL838X_SMI_ACCESS_PHY_CTRL_PARK_PAGE                   GENMASK(19, 15)
#define RTL838X_SMI_ACCESS_PHY_CTRL_PARK_PAGE_NO_PARK                   0x1f
#define RTL838X_SMI_ACCESS_PHY_CTRL_MAIN_PAGE                   GENMASK(14, 3)
#define RTL838X_SMI_ACCESS_PHY_CTRL_RWOP                        BIT(2) /* Write (1), Read (0) */
#define RTL838X_SMI_ACCESS_PHY_CTRL_TYPE                        BIT(1) /* Clause 45 (1), Clause 22 (0) */
#define RTL838X_SMI_ACCESS_PHY_CTRL_CMD                         BIT(0) /* Executes MDIO command, self-clears on completion */

#define RTL839X_SMI_ACCESS_PHY_CTRL_REG                 (0x03dc)
/* Reserved                                                     31 - 28 */
#define RTL839X_SMI_ACCESS_PHY_CTRL_PARK_PAGE                   GENMASK(27, 23)
#define RTL839X_SMI_ACCESS_PHY_CTRL_PARK_PAGE_NO_PARK                   0x1f
#define RTL839X_SMI_ACCESS_PHY_CTRL_MAIN_PAGE                   GENMASK(22, 10)
#define RTL839X_SMI_ACCESS_PHY_CTRL_REG_ADDR                    GENMASK(9, 5)
#define RTL839X_SMI_ACCESS_PHY_CTRL_BROADCAST                   BIT(4)
#define RTL839X_SMI_ACCESS_PHY_CTRL_RWOP                        BIT(3) /* Write (1), Read (0) */
#define RTL839X_SMI_ACCESS_PHY_CTRL_TYPE                        BIT(2) /* Clause 45 (1), Clause 22 (0) */
#define RTL839X_SMI_ACCESS_PHY_CTRL_FAIL                        BIT(1)
#define RTL839X_SMI_ACCESS_PHY_CTRL_CMD                         BIT(0) /* Executes MDIO command, self-clears on completion */

#define RTL930X_SMI_ACCESS_PHY_CTRL_REG                 (0xcb74)
/* Reserved                                                     31 - 26 */
#define RTL930X_SMI_ACCESS_PHY_CTRL_FAIL                        BIT(25)
#define RTL930X_SMI_ACCESS_PHY_CTRL_REG_ADDR                    GENMASK(24, 20)
#define RTL930X_SMI_ACCESS_PHY_CTRL_PARK_PAGE                   GENMASK(19, 15)
#define RTL930X_SMI_ACCESS_PHY_CTRL_PARK_PAGE_NO_PARK                   0x1f
#define RTL930X_SMI_ACCESS_PHY_CTRL_MAIN_PAGE                   GENMASK(14, 3)
#define RTL930X_SMI_ACCESS_PHY_CTRL_MAIN_PAGE_NO_CHANGE                 0xfff
#define RTL930X_SMI_ACCESS_PHY_CTRL_RWOP                        BIT(2) /* Write (1), Read (0) */
#define RTL930X_SMI_ACCESS_PHY_CTRL_TYPE                        BIT(1) /* Clause 45 (1), Clause 22 (0) */
#define RTL930X_SMI_ACCESS_PHY_CTRL_CMD                         BIT(0) /* Executes MDIO command, self-clears on completion */

#define RTL931X_SMI_ACCESS_PHY_CTRL_REG                 (0x0c00)
/* Reserved                                                     31 - 25 */
#define RTL931X_SMI_ACCESS_PHY_CTRL_ACCESS                      BIT(24)
#define RTL931X_SMI_ACCESS_PHY_CTRL_MAIN_PAGE                   GENMASK(23, 11)
#define RTL931X_SMI_ACCESS_PHY_CTRL_REG_ADDR                    GENMASK(10, 6)
#define RTL931X_SMI_ACCESS_PHY_CTRL_BROADCAST                   BIT(5)
#define RTL931X_SMI_ACCESS_PHY_CTRL_RWOP                        BIT(4) /* Write (1), Read (0) */
#define RTL931X_SMI_ACCESS_PHY_CTRL_TYPE                        GENMASK(3, 2)
#define RTL931X_SMI_ACCESS_PHY_CTRL_TYPE_C45_10G                        0x2
#define RTL931X_SMI_ACCESS_PHY_CTRL_TYPE_C45_1G                         0x1
#define RTL931X_SMI_ACCESS_PHY_CTRL_TYPE_C22                            0x0
#define RTL931X_SMI_ACCESS_PHY_CTRL_FAIL                        BIT(1)
#define RTL931X_SMI_ACCESS_PHY_CTRL_CMD                         BIT(0) /* Executes MDIO command, self-clears on completion */

#define RTL838X_SMI_PHY_DATA_CTRL_REG                   (0xa1c0)
/* INDATA becomes PORT_ID for read operations to select the port */
#define RTL838X_SMI_PHY_DATA_CTRL_PORT_ID                       GENMASK(20, 16)
#define RTL838X_SMI_PHY_DATA_CTRL_INDATA                        GENMASK(31, 16)
#define RTL838X_SMI_PHY_DATA_CTRL_DATA                          GENMASK(15, 0)

#define RTL839X_SMI_PHY_DATA_CTRL_REG                   (0x03f0)
/* INDATA becomes PORT_ID for read operations to select the port */
#define RTL839X_SMI_PHY_DATA_CTRL_PORT_ID                       GENMASK(20, 16)
#define RTL839X_SMI_PHY_DATA_CTRL_INDATA                        GENMASK(31, 16)
#define RTL839X_SMI_PHY_DATA_CTRL_DATA                          GENMASK(15, 0)

#define RTL930X_SMI_PHY_DATA_CTRL_REG                   (0xcb78)
/* INDATA becomes PORT_ID for read operations to select the port */
#define RTL930X_SMI_PHY_DATA_CTRL_PORT_ID                       GENMASK(20, 16)
#define RTL930X_SMI_PHY_DATA_CTRL_INDATA                        GENMASK(31, 16)
#define RTL930X_SMI_PHY_DATA_CTRL_DATA                          GENMASK(15, 0)

#define RTL931X_SMI_PHY_DATA_CTRL_REG                   (0x0c10)
#define RTL931X_SMI_PHY_DATA_CTRL_DATA                          GENMASK(31, 16)
#define RTL931X_SMI_PHY_DATA_CTRL_INDATA                        GENMASK(15, 0)

#define RTL838X_SMI_PHY_MMD_CTRL_REG                    (0xa1c4)
/* Reserved                                                 31 - 21 */
#define RTL838X_SMI_PHY_MMD_CTRL_DEVAD                      GENMASK(20, 16)
#define RTL838X_SMI_PHY_MMD_CTRL_REGAD                      GENMASK(15, 0)

#define RTL839X_SMI_PHY_MMD_CTRL_REG                    (0x03f4)
/* Reserved                                                     31 - 21 */
#define RTL839X_SMI_PHY_MMD_CTRL_DEVAD                          GENMASK(20, 16)
#define RTL839X_SMI_PHY_MMD_CTRL_REGAD                          GENMASK(15, 0)

#define RTL930X_SMI_PHY_MMD_CTRL_REG                    (0xcb7c)
/* Reserved                                                 31 - 21 */
#define RTL930X_SMI_PHY_MMD_CTRL_DEVAD                      GENMASK(20, 16)
#define RTL930X_SMI_PHY_MMD_CTRL_REGAD                      GENMASK(15, 0)

#define RTL931X_SMI_PHY_MMD_CTRL_REG                    (0x0c18)
/* Reserved                                                     31 - 21 */
#define RTL931X_SMI_PHY_MMD_CTRL_DEVAD                          GENMASK(20, 16)
#define RTL931X_SMI_PHY_MMD_CTRL_REGAD                          GENMASK(15, 0)

#define RTL931X_SMI_PHY_BC_PHYID_CTRL_REG               (0x0c14)
/* Reserved                                                     31 - 11 */
#define RTL931X_SMI_PHY_BC_PHYID_CTRL_PORT_ID                   GENMASK(10, 5)
#define RTL931X_SMI_PHY_BC_PHYID_CTRL_BC_PHYID                  GENMASK(4, 0)

#define RTL838X_SMI_PORT0_5_ADDR_CTRL	(0xa1c8)
#define RTL838X_SMI_POLL_CTRL		(0xa17c)

#define RTL839X_SMI_GLB_CTRL		(0x03f8)
#define RTL839X_SMI_PORT_POLLING_CTRL	(0x03fc)
#define RTL839X_PHYREG_ACCESS_CTRL	(0x03DC)
#define RTL839X_PHYREG_CTRL		(0x03E0)
#define RTL839X_PHYREG_PORT_CTRL	(0x03E4)
#define RTL839X_PHYREG_DATA_CTRL	(0x03F0)
#define RTL839X_PHYREG_MMD_CTRL		(0x3F4)

#define RTL930X_SMI_GLB_CTRL		(0xCA00)
#define RTL930X_SMI_POLL_CTRL		(0xca90)
#define RTL930X_SMI_PORT0_15_POLLING_SEL (0xCA08)
#define RTL930X_SMI_PORT16_27_POLLING_SEL (0xCA0C)
#define RTL930X_SMI_PORT0_5_ADDR	(0xCB80)
#define RTL930X_SMI_ACCESS_PHY_CTRL_0	(0xCB70)
#define RTL930X_SMI_ACCESS_PHY_CTRL_1	(0xCB74)
#define RTL930X_SMI_ACCESS_PHY_CTRL_2	(0xCB78)
#define RTL930X_SMI_ACCESS_PHY_CTRL_3	(0xCB7C)

#define RTL931X_SMI_GLB_CTRL1		(0x0CBC)
#define RTL931X_SMI_GLB_CTRL0		(0x0CC0)
#define RTL931X_SMI_PORT_POLLING_CTRL	(0x0CCC)
#define RTL931X_SMI_PORT_ADDR		(0x0C74)
#define RTL931X_SMI_PORT_POLLING_SEL	(0x0C9C)
#define RTL9310_SMI_PORT_POLLING_CTRL	(0x0CCC)
#define RTL931X_SMI_INDRT_ACCESS_CTRL_0	(0x0C00)
#define RTL931X_SMI_INDRT_ACCESS_CTRL_1	(0x0C04)
#define RTL931X_SMI_INDRT_ACCESS_CTRL_2	(0x0C08)
#define RTL931X_SMI_INDRT_ACCESS_CTRL_3	(0x0C10)
#define RTL931X_SMI_INDRT_ACCESS_BC_PHYID_CTRL (0x0C14)
#define RTL931X_SMI_INDRT_ACCESS_MMD_CTRL (0xC18)
#define RTL931X_MAC_L2_GLOBAL_CTRL2	(0x1358)
#define RTL931X_MAC_L2_GLOBAL_CTRL1	(0x5548)

/* Switch interrupts */
#define RTL838X_IMR_GLB			(0x1100)
#define RTL838X_IMR_PORT_LINK_STS_CHG	(0x1104)
#define RTL838X_ISR_GLB_SRC		(0x1148)
#define RTL838X_ISR_PORT_LINK_STS_CHG	(0x114C)

#define RTL839X_IMR_GLB			(0x0064)
#define RTL839X_IMR_PORT_LINK_STS_CHG	(0x0068)
#define RTL839X_ISR_GLB_SRC		(0x009c)
#define RTL839X_ISR_PORT_LINK_STS_CHG	(0x00a0)

#define RTL930X_IMR_GLB			(0xC628)
#define RTL930X_IMR_PORT_LINK_STS_CHG	(0xC62C)
#define RTL930X_ISR_GLB			(0xC658)
#define RTL930X_ISR_PORT_LINK_STS_CHG	(0xC660)

/* IMR_GLB does not exit on RTL931X */
#define RTL931X_IMR_PORT_LINK_STS_CHG	(0x126C)
#define RTL931X_ISR_GLB_SRC		(0x12B4)
#define RTL931X_ISR_PORT_LINK_STS_CHG	(0x12B8)

/* Definition of family IDs */
#define RTL8328_FAMILY_ID   (0x8328)
#define RTL8330_FAMILY_ID   (0x8330)
#define RTL8332_FAMILY_ID   (0x8332)
#define RTL8350_FAMILY_ID   (0x8350)
#define RTL8380_FAMILY_ID   (0x8380)
#define RTL8382_FAMILY_ID   (0x8382)
#define RTL8383_FAMILY_ID   (0x8383)
#define RTL8389_FAMILY_ID   (0x8389)
#define RTL8390_FAMILY_ID   (0x8390)
#define RTL8391_FAMILY_ID   (0x8391)
#define RTL8392_FAMILY_ID   (0x8392)
#define RTL8393_FAMILY_ID   (0x8393)
#define RTL9300_FAMILY_ID   (0x9300)
#define RTL9301_FAMILY_ID   (0x9301)
#define RTL9302_FAMILY_ID   (0x9302)
#define RTL9303_FAMILY_ID   (0x9303)
#define RTL9310_FAMILY_ID   (0x9310)
#define RTL9311_FAMILY_ID   (0x9311)
#define RTL9312_FAMILY_ID   (0x9312)
#define RTL9313_FAMILY_ID   (0x9313)

/* SPI Support */
#define RTL931X_SPI_CTRL0_REG                   (0x103C)
/* Reserved                                             31 - 26 */
#define RTL931X_SPI_CTRL0_DATA_WIDTH_MASK               GENMASK(25, 17)
#define RTL931X_SPI_CTRL0_ADDR_WIDTH_MASK               GENMASK(16, 14)
#define RTL931X_SPI_CTRL0_CLK_SEL_MASK                  GENMASK(13, 11)
#define RTL931X_SPI_CTRL0_CLK_SEL_DIV(d) \
        (((((d) + 1) / 2) - 1) & RTL931X_SPI_CTRL0_CLK_SEL_MASK) /* DIV16 - DIV2; 0x7 - 0x0 */
#define RTL931X_SPI_CTRL0_GPIO_SPI_SEL                  BIT(10)
#define RTL931X_SPI_CTRL0_GPIO12_CSB1_SEL               BIT(9)
#define RTL931X_SPI_CTRL0_GPIO11_CSB0_SEL               BIT(8)
#define RTL931X_SPI_CTRL0_CPHA                          BIT(7)
#define RTL931X_SPI_CTRL0_CPOL                          BIT(6)
#define RTL931X_SPI_CTRL0_TCLSH_MASK                    GENMASK(5, 3)
#define RTL931X_SPI_CTRL0_TCHSH_MASK                    GENMASK(2, 0)

/* Basic SoC Features */
/* Number of Ethernet ports */
#define RTL838X_PORT_ETH        24
#define RTL839X_PORT_ETH        48
#define RTL930X_PORT_ETH        26
#define RTL931X_PORT_ETH        52

/* Number of SFP ports */
#define RTL838X_PORT_SFP        4
#define RTL839X_PORT_SFP        4
#define RTL930X_PORT_SFP        2
#define RTL931X_PORT_SFP        0

/* Total port count */
#define RTL838X_PORT_CNT        (RTL838X_PORT_ETH + RTL838X_PORT_SFP)
#define RTL839X_PORT_CNT        (RTL839X_PORT_ETH + RTL839X_PORT_SFP)
#define RTL930X_PORT_CNT        (RTL930X_PORT_ETH + RTL930X_PORT_SFP)
#define RTL931X_PORT_CNT        (RTL931X_PORT_ETH + RTL931X_PORT_SFP)

/* CPU Port number */
#define RTL838X_PORT_CPU        RTL838X_PORT_CNT
#define RTL839X_PORT_CPU        RTL839X_PORT_CNT
#define RTL930X_PORT_CPU        RTL930X_PORT_CNT
#define RTL931X_PORT_CPU        RTL931X_PORT_CNT

/* RTL931x uses 4 ports for the internal CPU port */
#define RTL931X_INT_SERDES_PORTS        4

/* Last port is the port count + the cpu port */
#define RTL838X_PORT_END        (RTL838X_PORT_CNT + 1)
#define RTL839X_PORT_END        (RTL839X_PORT_CNT + 1)
#define RTL930X_PORT_END        (RTL930X_PORT_CNT + 1)
#define RTL931X_PORT_END        (RTL931X_PORT_CNT + RTL931X_INT_SERDES_PORTS)

struct rtl83xx_soc_info {
	unsigned char *name;
	unsigned int id;
	unsigned int family;
	unsigned char *compatible;
	volatile void *sw_base;
	volatile void *icu_base;
	int cpu_port;
};

/* rtl83xx-related functions used across subsystems */
int rtl838x_smi_wait_op(int timeout);
int rtl838x_read_phy(u32 port, u32 page, u32 reg, u32 *val);
int rtl838x_write_phy(u32 port, u32 page, u32 reg, u32 val);
int rtl839x_read_phy(u32 port, u32 page, u32 reg, u32 *val);
int rtl839x_write_phy(u32 port, u32 page, u32 reg, u32 val);
int rtl930x_read_phy(u32 port, u32 page, u32 reg, u32 *val);
int rtl930x_write_phy(u32 port, u32 page, u32 reg, u32 val);
int rtl931x_read_phy(u32 port, u32 page, u32 reg, u32 *val);
int rtl931x_write_phy(u32 port, u32 page, u32 reg, u32 val);

#endif   /* _MACH_REALTEK_OTTO_H_ */
