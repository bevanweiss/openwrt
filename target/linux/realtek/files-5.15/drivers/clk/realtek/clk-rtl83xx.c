// SPDX-License-Identifier: GPL-2.0-only
/*
 * Realtek RTL83XX clock driver
 * Copyright (C) 2022 Markus Stockhausen <markus.stockhausen@gmx.de>
 *
 */

#include <asm/addrspace.h>
#include <asm/cacheflush.h>
#include <asm/mipsmtregs.h>
#include <dt-bindings/clock/rtl83xx-clk.h>
#include <linux/bitops.h>
#include <linux/bitfield.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/clkdev.h>
#include <linux/cpu.h>
#include <linux/delay.h>
#include <linux/genalloc.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include "clk-rtl83xx.h"

#define RTL_SOC_BASE                    (0x18000000)

#define RTL930X_SYS_PLL_CTRL_REG                (0x0200)
/* Reserved                                             31 - 22 */
#define RTL930X_SYS_PLL_CTRL_OCP0_PLL_DIV               GENMASK(21, 16)
/* Reserved                                             15 - 6 */
#define RTL930X_SYS_PLL_CTRL_SD_PLL_DIV                 GENMASK(5, 0)

#define RTL930X_CCR_PLL_CTRL_REG                (0x0204)
/* Reserved                                             31 */
#define RTL930X_CCR_PLL_CTRL_SSC_nRST                   BIT(30)
#define RTL930X_CCR_PLL_CTRL_SSC_TEST_MODE              GENMASK(29, 28)
#define RTL930X_CCR_PLL_CTRL_SSC_OFFSET                 GENMASK(27, 20)
#define RTL930X_CCR_PLL_CTRL_SSC_STEP                   GENMASK(19, 14)
#define RTL930X_CCR_PLL_CTRL_SSC_PERIOD                 GENMASK(13, 7)
#define RTL930X_CCR_PLL_CTRL_SSC_EN                     BIT(6)
/* Reserved                                             5 */
#define RTL930X_CCR_PLL_CTRL_DDR_PWRDN                  BIT(4)
#define RTL930X_CCR_PLL_CTRL_FRAC_EN                    BIT(3)
#define RTL930X_CCR_PLL_CTRL_DDR_FUPDN                  BIT(2)
#define RTL930X_CCR_PLL_CTRL_DDR_PSEN                   BIT(1)
#define RTL930X_CCR_PLL_CTRL_DDR_OEB                    BIT(0)

#define RTL930X_CCR_PLL0_CTRL_REG               (0x0208)
/* Unknown, default/expected value 0x1cda9 */

#define RTL930X_CCR_PLL1_CTRL_REG               (0x020c)
/* Unknown, default/expected value 0x7e95a439 */

#define RTL930X_CCR_PLL2_CTRL_REG               (0x0210)
/* Unknown, default/expected value 0xf0ffe8 */

#define RTL930X_CCR_DRCK0_CTRL_REG              (0x021c)
/* Reserved                                             31 - 21 */
#define RTL930X_CCR_DRCK0_DDR_CK_M_90_TAP               GENMASK(20, 16)
/* Reserved                                             15 - 13 */
#define RTL930X_CCR_DRCK0_DDR_CK_M_TAP                  GENMASK(12, 8)
/* Reserved                                             7 - 5 */
#define RTL930X_CCR_DRCK0_DDR_CK_PHS_SEL                GENMASK(4, 0)

#define RTL930X_CCR_MCKG_PHS_SEL_REG            (0x0220)
/* Reserved                                             31 - 3 */
#define RTL930X_CCR_MCKG_PHS_SEL_PHASE_SEL              GENMASK(2, 0)

#define RTL930X_CCR_MCKG_DIV_SEL_REG            (0x0224)
/* Reserved                                             31 - 2 */
#define RTL930X_CCR_MCK_DIV_SEL_DIVL                    GENMASK(1, 0)

#define RTL930X_CCR_LX_CLK_PLL_REG              (0x0228)
/* Reserved                                             31 - 12 */
#define RTL930X_CCR_LX_CLK_PLL_SPI_FLASH_CLK_DIV        GENMASK(11, 8)
/* Reserved                                             7 - 4 */
#define RTL930X_CCR_LX_CLK_PLL_DIV                      GENMASK(3, 0)

#define RTL930X_CCR_DRAM_CLK_EN_REG             (0x022c)
/* Reserved                                             31 - 1 */
#define RTL930X_CCR_DRAM_CLK_EN_START_CLK_OUT           BIT(0)

#define RTL930X_CCR_DRAM_CLK_CHG_REG            (0x0230)
/* Reserved                                             31 - 1 */
#define RTL930X_CCR_DRAM_CLK_CHG_DG_EN                  BIT(0)

#define RTL930X_PLL_MEM_CTRL0_REG               (0x0234)
#define RTL930X_PLL_MEM_CTRL1_REG               (0x0238)

#define RTL930X_PLL_MEM_CTRL2_REG               (0x023c)
/* Reserved                                             31 - 16 */
#define RTL930X_PLL_MEM_CTRL2_PREDIV                    GENMASK(15, 14)
#define RTL930X_PLL_MEM_CTRL2_PREDIV_DIV(reg) \
        (1 << (reg))
/* Reserved                                             13 - 0 */

#define RTL930X_PLL_MEM_CTRL3_REG               (0x0240)
#define RTL930X_PLL_MEM_CTRL3_CMU_NCODE_IN              GENMASK(31, 24)
#define RTL930X_PLL_MEM_CTRL3_CMU_NCODE_IN_CODE(reg) \
        ((reg) + 2)
/* Reserved                                             23 - 0 */

#define RTL930X_PLL_MEM_CTRL4_REG               (0x0244)
#define RTL930X_PLL_MEM_CTRL5_REG               (0x0248)
#define RTL930X_PLL_MEM_CTRL6_REG               (0x024c)

#define RTL_SW_CORE_BASE                (0x1b000000)

/*
 * some hardware specific definitions
 */

#define SOC_RTL838X		0
#define SOC_RTL839X		1
#define SOC_RTL930X		2
#define SOC_COUNT		3

#define MEM_DDR1		1
#define MEM_DDR2		2
#define MEM_DDR3		3

#define REG_CTRL0		0
#define REG_CTRL1		1
#define REG_COUNT		2

#define OSC_RATE		25000000

#define RTCL_SOC_CLK(soc,clk)	((soc << 8) + clk)

static const int rtcl_regs[SOC_COUNT][REG_COUNT][CLK_COUNT] = {
	{
		{
			RTL_SW_CORE_BASE + RTL838X_PLL_SW_CTRL0_REG,
			RTL_SW_CORE_BASE + RTL838X_PLL_CPU_CTRL0_REG,
			RTL_SW_CORE_BASE + RTL838X_PLL_MEM_CTRL0_REG,
			RTL_SW_CORE_BASE + RTL838X_PLL_LXB_CTRL0_REG,
		}, {
			RTL_SW_CORE_BASE + RTL838X_PLL_SW_CTRL1_REG,
			RTL_SW_CORE_BASE + RTL838X_PLL_CPU_CTRL1_REG,
			RTL_SW_CORE_BASE + RTL838X_PLL_MEM_CTRL1_REG,
			RTL_SW_CORE_BASE + RTL838X_PLL_LXB_CTRL1_REG,
		},
	}, {
		{
			RTL_SW_CORE_BASE + RTL839X_PLL_SW_CTRL_REG,
			RTL_SW_CORE_BASE + RTL839X_PLL_CPU_CTRL0_REG,
			RTL_SW_CORE_BASE + RTL839X_PLL_MEM_CTRL0_REG,
			RTL_SW_CORE_BASE + RTL839X_PLL_LXB_CTRL0_REG,
		}, {
			RTL_SW_CORE_BASE + RTL839X_PLL_SW_CTRL_REG,
			RTL_SW_CORE_BASE + RTL839X_PLL_CPU_CTRL1_REG,
			RTL_SW_CORE_BASE + RTL839X_PLL_MEM_CTRL1_REG,
			RTL_SW_CORE_BASE + RTL839X_PLL_LXB_CTRL1_REG,
		},
	}, {
		{
			RTL_SW_CORE_BASE + RTL930X_PLL_SW_CTRL0_REG,
			RTL_SW_CORE_BASE + RTL930X_PLL_CPU_CTRL0_REG,
			RTL_SOC_BASE + RTL930X_PLL_MEM_CTRL2_REG,
			RTL_SW_CORE_BASE + RTL930X_PLL_SW_DIV_CTRL_REG,
		}, {
			RTL_SW_CORE_BASE + RTL930X_PLL_SW_CTRL1_REG,
			RTL_SW_CORE_BASE + RTL930X_PLL_CPU_MISC_CTRL_REG,
			RTL_SOC_BASE + RTL930X_PLL_MEM_CTRL3_REG,
			RTL_SW_CORE_BASE + RTL930X_PLL_SW_DIV_CTRL_REG,
		},
	}
};

#define RTL_OTTO_PLL_SET(_rate, _divn2, _ncode_in, _sel, _selb)               \
	{                                                                     \
		.rate = _rate,                                                \
		.divn2 = _divn2,                                              \
		.ncode_in = _ncode_in,                                        \
		.selb = _selb,                                                \
	}

struct rtcl_reg_set {
	unsigned int rate;
	u32 divn2;
	u32 ncode_in;
	bool selb;
};

/*
 * The following configuration tables are valid operation points for their
 * corresponding PLLs. The magic numbers are precalculated mulitpliers and
 * dividers to keep the driver simple. They also provide rates outside the
 * allowed physical specifications. E.g. DDR3 memory has a lower limit of 303
 * MHz or the CPU might get unstable if set to anything above its startup
 * frequency. Additionally the Realtek SOCs tend to expect CPU speed larger
 * than MEM speed larger than LXB speed. The caller or DT configuration must
 * take care that only valid operating points are selected.
 */

static const struct rtcl_reg_set rtcl_838x_cpu_reg_set[] = {
	RTL_OTTO_PLL_SET(300000000, 0x04, 0x5c, 0x2, true),
	RTL_OTTO_PLL_SET(325000000, 0x04, 0x64, 0x2, true),
	RTL_OTTO_PLL_SET(350000000, 0x04, 0x6c, 0x2, true),
	RTL_OTTO_PLL_SET(375000000, 0x04, 0x74, 0x2, true),
	RTL_OTTO_PLL_SET(400000000, 0x04, 0x5c, 0x1, true),
	RTL_OTTO_PLL_SET(425000000, 0x04, 0x62, 0x1, true),
	RTL_OTTO_PLL_SET(450000000, 0x04, 0x68, 0x1, true),
	RTL_OTTO_PLL_SET(475000000, 0x04, 0x6e, 0x1, true),
	RTL_OTTO_PLL_SET(500000000, 0x04, 0x74, 0x1, true),
	RTL_OTTO_PLL_SET(525000000, 0x04, 0x7a, 0x1, true),
	RTL_OTTO_PLL_SET(550000000, 0x04, 0x80, 0x1, true),
	RTL_OTTO_PLL_SET(575000000, 0x04, 0x86, 0x1, true),
	RTL_OTTO_PLL_SET(600000000, 0x04, 0x8c, 0x1, true),
	RTL_OTTO_PLL_SET(625000000, 0x04, 0x92, 0x1, true),
};

static const struct rtcl_reg_set rtcl_838x_mem_reg_set[] = {
	RTL_OTTO_PLL_SET(200000000, 0x04, 0x1b, 0x2, true),
	RTL_OTTO_PLL_SET(225000000, 0x04, 0x17, 0x1, true),
	RTL_OTTO_PLL_SET(250000000, 0x04, 0x1a, 0x1, true),
	RTL_OTTO_PLL_SET(275000000, 0x04, 0x12, 0x0, true),
	RTL_OTTO_PLL_SET(300000000, 0x04, 0x14, 0x0, true),
	RTL_OTTO_PLL_SET(325000000, 0x04, 0x16, 0x0, true),
	RTL_OTTO_PLL_SET(350000000, 0x04, 0x18, 0x0, true),
	RTL_OTTO_PLL_SET(375000000, 0x04, 0x1a, 0x0, true),
};

static const struct rtcl_reg_set rtcl_838x_lxb_reg_set[] = {
	RTL_OTTO_PLL_SET(100000000, 0x04, 0x3c, 0x0, false),
	RTL_OTTO_PLL_SET(125000000, 0x04, 0x3c, 0x0, false),
	RTL_OTTO_PLL_SET(150000000, 0x04, 0x50, 0x2, true),
	RTL_OTTO_PLL_SET(175000000, 0x04, 0x50, 0x2, true),
	RTL_OTTO_PLL_SET(200000000, 0x04, 0x7c, 0x0, false),
};

static const struct rtcl_reg_set rtcl_839x_cpu_reg_set[] = {
	RTL_OTTO_PLL_SET(400000000, 0x04, 0x14, 0x2, true),
	RTL_OTTO_PLL_SET(425000000, 0x04, 0x1e, 0x3, false),
	RTL_OTTO_PLL_SET(450000000, 0x04, 0x17, 0x2, true),
	RTL_OTTO_PLL_SET(475000000, 0x04, 0x22, 0x3, false),
	RTL_OTTO_PLL_SET(500000000, 0x04, 0x1a, 0x2, true),
	RTL_OTTO_PLL_SET(525000000, 0x04, 0x26, 0x3, false),
	RTL_OTTO_PLL_SET(550000000, 0x04, 0x12, 0x2, false),
	RTL_OTTO_PLL_SET(575000000, 0x04, 0x2a, 0x3, false),
	RTL_OTTO_PLL_SET(600000000, 0x04, 0x14, 0x2, false),
	RTL_OTTO_PLL_SET(625000000, 0x04, 0x2e, 0x3, false),
	RTL_OTTO_PLL_SET(650000000, 0x04, 0x16, 0x2, false),
	RTL_OTTO_PLL_SET(675000000, 0x04, 0x32, 0x3, false),
	RTL_OTTO_PLL_SET(700000000, 0x04, 0x18, 0x2, false),
	RTL_OTTO_PLL_SET(725000000, 0x04, 0x36, 0x3, false),
	RTL_OTTO_PLL_SET(750000000, 0x04, 0x38, 0x3, false),
	RTL_OTTO_PLL_SET(775000000, 0x04, 0x3a, 0x3, false),
	RTL_OTTO_PLL_SET(800000000, 0x04, 0x3c, 0x3, false),
	RTL_OTTO_PLL_SET(825000000, 0x04, 0x3e, 0x3, false),
	RTL_OTTO_PLL_SET(850000000, 0x04, 0x40, 0x3, false),
};

static const struct rtcl_reg_set rtcl_839x_mem_reg_set[] = {
	RTL_OTTO_PLL_SET(125000000, 0x04, 0x1a, 0x3, true),
	RTL_OTTO_PLL_SET(150000000, 0x04, 0x14, 0x3, false),
	RTL_OTTO_PLL_SET(175000000, 0x04, 0x18, 0x3, false),
	RTL_OTTO_PLL_SET(200000000, 0x04, 0x1c, 0x3, false),
	RTL_OTTO_PLL_SET(225000000, 0x04, 0x17, 0x2, true),
	RTL_OTTO_PLL_SET(250000000, 0x04, 0x1a, 0x2, true),
	RTL_OTTO_PLL_SET(275000000, 0x04, 0x12, 0x2, true),
	RTL_OTTO_PLL_SET(300000000, 0x04, 0x14, 0x2, false),
	RTL_OTTO_PLL_SET(325000000, 0x04, 0x16, 0x2, false),
	RTL_OTTO_PLL_SET(350000000, 0x04, 0x18, 0x2, false),
	RTL_OTTO_PLL_SET(375000000, 0x04, 0x1a, 0x2, false),
	RTL_OTTO_PLL_SET(400000000, 0x04, 0x1c, 0x2, false),
};

static const struct rtcl_reg_set rtcl_839x_lxb_reg_set[] = {
	RTL_OTTO_PLL_SET( 50000000, 0x14, 0x14, 0x1, false),
	RTL_OTTO_PLL_SET(100000000, 0x08, 0x14, 0x1, false),
	RTL_OTTO_PLL_SET(150000000, 0x04, 0x14, 0x1, false),
	RTL_OTTO_PLL_SET(200000000, 0x04, 0x14, 0x3, true),
};

static const struct rtcl_reg_set rtcl_930x_cpu_reg_set[] = {
/*
 * TODO: The SRAM runs on 250 MHz. Depending on the the CPU speed we need
 * additional magic register settings to make things work. For now remember
 * that, provide all PLL values and exclude unwanted settings in DT.
 */
	RTL_OTTO_PLL_SET( 400000000, 0x4, 0x2e, 0x0, false), /* sram_oc0_pulse=1 oc0_sram_pulse=0 */
	RTL_OTTO_PLL_SET( 425000000, 0x4, 0x31, 0x0, false), /* ... */
	RTL_OTTO_PLL_SET( 450000000, 0x4, 0x34, 0x0, false),
	RTL_OTTO_PLL_SET( 475000000, 0x4, 0x37, 0x0, false), /* sram_oc0_pulse=1 oc0_sram_pulse=0 */
	RTL_OTTO_PLL_SET( 500000000, 0x4, 0x3a, 0x0, false), /* sram_oc0_pulse=1 oc0_sram_pulse=1 */
	RTL_OTTO_PLL_SET( 525000000, 0x3, 0x32, 0x0, false), /* sram_oc0_pulse=0 oc0_sram_pulse=1 */
	RTL_OTTO_PLL_SET( 550000000, 0x3, 0x35, 0x0, false), /* ... */
	RTL_OTTO_PLL_SET( 575000000, 0x3, 0x37, 0x0, false),
	RTL_OTTO_PLL_SET( 600000000, 0x2, 0x2e, 0x0, false),
	RTL_OTTO_PLL_SET( 625000000, 0x2, 0x30, 0x0, false),
	RTL_OTTO_PLL_SET( 650000000, 0x2, 0x32, 0x0, false),
	RTL_OTTO_PLL_SET( 675000000, 0x2, 0x34, 0x0, false),
	RTL_OTTO_PLL_SET( 700000000, 0x2, 0x36, 0x0, false),
	RTL_OTTO_PLL_SET( 725000000, 0x2, 0x38, 0x0, false),
	RTL_OTTO_PLL_SET( 750000000, 0x2, 0x3a, 0x0, false),
	RTL_OTTO_PLL_SET( 775000000, 0x2, 0x3c, 0x0, false),
	RTL_OTTO_PLL_SET( 800000000, 0x2, 0x3e, 0x0, false),
	RTL_OTTO_PLL_SET( 825000000, 0x2, 0x40, 0x0, false),
	RTL_OTTO_PLL_SET( 850000000, 0x2, 0x42, 0x0, false),
	RTL_OTTO_PLL_SET( 875000000, 0x2, 0x44, 0x0, false),
	RTL_OTTO_PLL_SET( 900000000, 0x2, 0x46, 0x0, false),
	RTL_OTTO_PLL_SET( 925000000, 0x2, 0x48, 0x0, false),
	RTL_OTTO_PLL_SET( 950000000, 0x2, 0x4a, 0x0, false),
	RTL_OTTO_PLL_SET( 975000000, 0x2, 0x4c, 0x0, false), /* sram_oc0_pulse=0 oc0_sram_pulse=1 */
	RTL_OTTO_PLL_SET(1000000000, 0x2, 0x4e, 0x0, false), /* sram_oc0_pulse=0 oc0_sram_pulse=2 */
	RTL_OTTO_PLL_SET(1025000000, 0x2, 0x50, 0x0, false), /* ... */
};

static const struct rtcl_reg_set rtcl_930x_mem_reg_set[] = {
	RTL_OTTO_PLL_SET(600000000, 0x0, 0x00, 0x0, false),
};

static const struct rtcl_reg_set rtcl_930x_lxb_reg_set[] = {
	RTL_OTTO_PLL_SET(153125000, 0x0, 0x0c00, 0x0, false), /* FCODE_IN, currently unused */
	RTL_OTTO_PLL_SET(175000000, 0x0, 0x0a00, 0x0, false), /* FCODE_IN, currently unused */
};

struct rtcl_rtab_set {
	int count;
	const struct rtcl_reg_set *rset;
};

#define RTCL_RTAB_SET(_rset)			\
	{					\
		.count = ARRAY_SIZE(_rset),	\
		.rset = _rset,			\
	}

#define RTCL_RTAB_SET_NONE                                                    \
	{                                                                     \
		.count = 0,                                                   \
		.rset = NULL,                                                 \
	}

static const struct rtcl_rtab_set rtcl_rtab_set[SOC_COUNT][CLK_COUNT] = {
	{
		RTCL_RTAB_SET_NONE,
		RTCL_RTAB_SET(rtcl_838x_cpu_reg_set),
		RTCL_RTAB_SET(rtcl_838x_mem_reg_set),
		RTCL_RTAB_SET(rtcl_838x_lxb_reg_set)
	}, {
		RTCL_RTAB_SET_NONE,
		RTCL_RTAB_SET(rtcl_839x_cpu_reg_set),
		RTCL_RTAB_SET(rtcl_839x_mem_reg_set),
		RTCL_RTAB_SET(rtcl_839x_lxb_reg_set)
	}, {
		RTCL_RTAB_SET_NONE,
		RTCL_RTAB_SET(rtcl_930x_cpu_reg_set),
		RTCL_RTAB_SET(rtcl_930x_mem_reg_set),
		RTCL_RTAB_SET(rtcl_930x_lxb_reg_set)
	}
};

#define RTCL_ROUND_SET(_min, _max, _step)	\
	{					\
		.min = _min,			\
		.max = _max,			\
		.step = _step,			\
	}

#define RTCL_ROUND_SET_NONE \
        RTCL_ROUND_SET(0, 0, 1)

struct rtcl_round_set {
	unsigned long min;
	unsigned long max;
	unsigned long step;
};

static const struct rtcl_round_set rtcl_round_set[SOC_COUNT][CLK_COUNT] = {
	{
		RTCL_ROUND_SET_NONE,
		RTCL_ROUND_SET(300000000, 625000000, 25000000),
		RTCL_ROUND_SET(200000000, 375000000, 25000000),
		RTCL_ROUND_SET(100000000, 200000000, 25000000)
	}, {
		RTCL_ROUND_SET_NONE,
		RTCL_ROUND_SET(400000000, 850000000, 25000000),
		RTCL_ROUND_SET(100000000, 400000000, 25000000),
		RTCL_ROUND_SET(50000000, 200000000, 50000000)
	}, {
		RTCL_ROUND_SET_NONE,
		RTCL_ROUND_SET(400000000, 975000000, 25000000),
		RTCL_ROUND_SET(600000000, 600000000, 1),
		RTCL_ROUND_SET(153125000, 175000000, 21875000),
	}
};

/*
 * module data structures
 */

#define RTCL_CLK_INFO(_idx, _name, _pname0, _pname1, _pname2, _dname) \
	{						\
		.idx = _idx,				\
		.name = _name,				\
		.parent_name[SOC_RTL838X] = _pname0,	\
		.parent_name[SOC_RTL839X] = _pname1,	\
		.parent_name[SOC_RTL930X] = _pname2,	\
		.display_name = _dname,			\
	}

struct rtcl_clk_info {
	unsigned int idx;
	const char *name;
	const char *parent_name[SOC_COUNT];
	const char *display_name;
};

struct rtcl_clk {
	struct clk_hw hw;
	unsigned int idx;
	unsigned long min;
	unsigned long max;
	unsigned long startup;
};

/*
 * PLLs on the devices are mostly pretty straight forward derived from 25 MHz
 * oscillator clock. Only exception is RTL930X where LXB clock has only dividers
 * and is derived from switch clock.
 */
static const struct rtcl_clk_info rtcl_clk_info[CLK_COUNT] = {
	RTCL_CLK_INFO(CLK_SW, "swcore_clk", "ref_clk", "ref_clk", "ref_clk", "SW"),
	RTCL_CLK_INFO(CLK_CPU, "cpu_clk", "ref_clk", "ref_clk", "ocp_clk", "CPU"),
	RTCL_CLK_INFO(CLK_MEM, "ddr_clk", "ref_clk", "ref_clk", "mem_clk", "MEM"),
	RTCL_CLK_INFO(CLK_LXB, "lx_clk", "ref_clk", "ref_clk", "swcore_clk", "LXB")
};

struct rtcl_sram {
	int *pmark;
	unsigned long vbase;
};

struct rtcl_ccu {
	spinlock_t lock;
	unsigned int soc;
	struct rtcl_sram sram;
	struct device_node *np;
	struct platform_device *pdev;
	struct rtcl_clk clks[CLK_COUNT];
};

struct rtcl_ccu *rtcl_ccu;

#define rtcl_hw_to_clk(_hw) container_of(_hw, struct rtcl_clk, hw)

/*
 * SRAM relocatable assembler functions. The dram() parts point to normal kernel
 * memory while the sram() parts are the same functions but relocated to SRAM.
 */

extern void	rtcl_838x_dram_start(void);
extern int	rtcl_838x_dram_size;

extern void	rtcl_838x_dram_set_rate(u32 clk_idx, u32 ctrl0, u32 ctrl1);
static void	(*rtcl_838x_sram_set_rate)(u32 clk_idx, u32 ctrl0, u32 ctrl1);

extern void	rtcl_839x_dram_start(void);
extern int	rtcl_839x_dram_size;

extern void	rtcl_839x_dram_set_rate(u32 clk_idx, u32 ctrl0, u32 ctrl1);
static void	(*rtcl_839x_sram_set_rate)(u32 clk_idx, u32 ctrl0, u32 ctrl1);

extern void	rtcl_930x_dram_start(void);
extern int	rtcl_930x_dram_size;

extern void	rtcl_930x_dram_set_rate(int clk_idx, u32 ctrl0, u32 ctrl1);
static void	(*rtcl_930x_sram_set_rate)(int clk_idx, u32 ctrl0, u32 ctrl1);

/*
 * clock setter/getter functions
 */

static unsigned long rtcl_recalc_rate(struct clk_hw *hw, unsigned long parent_rate)
{
	struct rtcl_clk *clk = rtcl_hw_to_clk(hw);
	unsigned int ctrl0, ctrl1;
	unsigned int mul1 = 1, mul2 = 1, div1 = 1, div2 = 1, div3 = 1;
	unsigned int cmu_divn2, cmu_divn2_selb, cmu_divn3_sel;

	if ((clk->idx >= CLK_COUNT) || (!rtcl_ccu) || (rtcl_ccu->soc >= SOC_COUNT))
		return 0;

	ctrl0 = ioread32((void __iomem *)CKSEG1ADDR(rtcl_regs[rtcl_ccu->soc][REG_CTRL0][clk->idx]));
	ctrl1 = ioread32((void __iomem *)CKSEG1ADDR(rtcl_regs[rtcl_ccu->soc][REG_CTRL1][clk->idx]));

	switch (RTCL_SOC_CLK(rtcl_ccu->soc, clk->idx)) {
	case RTCL_SOC_CLK(SOC_RTL838X, CLK_CPU):
		div3 = 2;
		fallthrough;
	case RTCL_SOC_CLK(SOC_RTL838X, CLK_LXB):
		div3 = 2;
		fallthrough;
	case RTCL_SOC_CLK(SOC_RTL838X, CLK_MEM):
		cmu_divn2 = RTL838X_PLL_CMU_CTRL0_DIVN2_DIV(FIELD_GET(RTL838X_PLL_CMU_CTRL0_DIVN2, ctrl0));
		cmu_divn2_selb = FIELD_GET(RTL838X_PLL_CMU_CTRL1_DIVN2_SELB, ctrl1);
		cmu_divn3_sel = RTL838X_PLL_CMU_CTRL1_DIVN3_SEL_DIV(FIELD_GET(RTL838X_PLL_CMU_CTRL1_DIVN3_SEL, ctrl1));

		mul1 = RTL838X_PLL_CMU_CTRL0_NCODE_IN_CODE(FIELD_GET(RTL838X_PLL_CMU_CTRL0_NCODE_IN, ctrl0));
		mul2 = RTL838X_PLL_CMU_CTRL0_SEL_DIV4_DIV(FIELD_GET(RTL838X_PLL_CMU_CTRL0_SEL_DIV4, ctrl0));
		div1 = RTL838X_PLL_CMU_CTRL0_SEL_PREDIV_DIV((FIELD_GET(RTL838X_PLL_CMU_CTRL0_SEL_PREDIV, ctrl0)));
		div2 = cmu_divn2_selb ? cmu_divn3_sel : cmu_divn2;
		div3 = 4;
		break;
	case RTCL_SOC_CLK(SOC_RTL838X, CLK_SW):
		mul1 = RTL838X_PLL_CMU_CTRL0_NCODE_IN_CODE(FIELD_GET(RTL838X_PLL_CMU_CTRL0_NCODE_IN, ctrl0));
		mul2 = RTL838X_PLL_CMU_CTRL0_SEL_DIV4_DIV(FIELD_GET(RTL838X_PLL_CMU_CTRL0_SEL_DIV4, ctrl0));
		div1 = RTL838X_PLL_CMU_CTRL0_SEL_PREDIV_DIV((FIELD_GET(RTL838X_PLL_CMU_CTRL0_SEL_PREDIV, ctrl0)));
		div3 = 1;
		break;
	case RTCL_SOC_CLK(SOC_RTL839X, CLK_CPU):
		div3 = 2;
		fallthrough;
	case RTCL_SOC_CLK(SOC_RTL839X, CLK_LXB):
		div3 = 2;
		fallthrough;
	case RTCL_SOC_CLK(SOC_RTL839X, CLK_MEM):
		cmu_divn2 = RTL839X_PLL_CMU_CTRL0_DIVN2_DIV(FIELD_GET(RTL839X_PLL_CMU_CTRL0_DIVN2, ctrl0));
		cmu_divn2_selb = FIELD_GET(RTL839X_PLL_CMU_CTRL1_DIVN2_SELB, ctrl1);
		cmu_divn3_sel = RTL839X_PLL_CMU_CTRL1_DIVN3_SEL_DIV(FIELD_GET(RTL839X_PLL_CMU_CTRL1_DIVN3_SEL, ctrl1));

		mul1 = RTL839X_PLL_CMU_CTRL0_NCODE_IN_CODE(FIELD_GET(RTL839X_PLL_CMU_CTRL0_NCODE_IN, ctrl0));
		mul2 = RTL839X_PLL_CMU_CTRL0_SEL_DIV4_DIV(FIELD_GET(RTL839X_PLL_CMU_CTRL0_SEL_DIV4, ctrl0));
		div1 = RTL839X_PLL_CMU_CTRL0_SEL_PREDIV_DIV(FIELD_GET(RTL839X_PLL_CMU_CTRL0_SEL_PREDIV, ctrl0));
		div2 = cmu_divn2_selb ? cmu_divn3_sel : cmu_divn2;
		div3 = 4;
		break;
	case RTCL_SOC_CLK(SOC_RTL839X, CLK_SW):
		mul1 = RTL839X_PLL_CMU_CTRL0_NCODE_IN_CODE(FIELD_GET(RTL839X_PLL_CMU_CTRL0_NCODE_IN, ctrl0));
		mul2 = RTL839X_PLL_CMU_CTRL0_SEL_DIV4_DIV(FIELD_GET(RTL839X_PLL_CMU_CTRL0_SEL_DIV4, ctrl0));
		div1 = RTL839X_PLL_CMU_CTRL0_SEL_PREDIV_DIV(FIELD_GET(RTL839X_PLL_CMU_CTRL0_SEL_PREDIV, ctrl0));
		div3 = 1;
		break;
	case RTCL_SOC_CLK(SOC_RTL930X, CLK_CPU):
		mul1 = RTL930X_PLL_CMU_CTRL0_NCODE_IN_CODE(FIELD_GET(RTL930X_PLL_CMU_CTRL0_NCODE_IN, ctrl0));
		mul2 = RTL930X_PLL_CMU_CTRL0_SEL_DIV4_DIV(FIELD_GET(RTL930X_PLL_CMU_CTRL0_SEL_DIV4, ctrl0));
		div1 = RTL930X_PLL_CMU_CTRL0_SEL_PREDIV_DIV(FIELD_GET(RTL930X_PLL_CMU_CTRL0_SEL_PREDIV, ctrl0));
		div2 = RTL930X_PLL_CPU_MISC_CTRL_DIVN2_CPU_DIV(FIELD_GET(RTL930X_PLL_CPU_MISC_CTRL_DIVN2_CPU, ctrl1));
		div3 = RTL930X_PLL_CPU_CTRL0_DIVN3_DIV(FIELD_GET(RTL930X_PLL_CPU_CTRL0_DIVN3, ctrl0));
		break;
	case RTCL_SOC_CLK(SOC_RTL930X, CLK_LXB):
		div1 = RTL930X_PLL_SW_DIV_CTRL_DIVN2_LXB_NAND_USB_DIV(FIELD_GET(RTL930X_PLL_SW_DIV_CTRL_DIVN2_LXB_NAND_USB, ctrl0));
		break;
	case RTCL_SOC_CLK(SOC_RTL930X, CLK_MEM):
		mul1 = RTL930X_PLL_MEM_CTRL3_CMU_NCODE_IN_CODE(FIELD_GET(RTL930X_PLL_MEM_CTRL3_CMU_NCODE_IN, ctrl1));
		div1 = RTL930X_PLL_MEM_CTRL2_PREDIV_DIV(FIELD_GET(RTL930X_PLL_MEM_CTRL2_PREDIV, ctrl0));
		div2 = 2;
		break;
	case RTCL_SOC_CLK(SOC_RTL930X, CLK_SW):
		mul1 = RTL930X_PLL_CMU_CTRL0_NCODE_IN_CODE(FIELD_GET(RTL930X_PLL_CMU_CTRL0_NCODE_IN, ctrl0));
		mul2 = RTL930X_PLL_CMU_CTRL0_SEL_DIV4_DIV(FIELD_GET(RTL930X_PLL_CMU_CTRL0_SEL_DIV4, ctrl0));
		div1 = RTL930X_PLL_CMU_CTRL0_SEL_PREDIV_DIV(FIELD_GET(RTL930X_PLL_CMU_CTRL0_SEL_PREDIV, ctrl0));
		break;
	}

	/* Ensure all intermittent values always fit in 32 bits by shifting 4 */
	return ((((parent_rate >> 4) * mul1) / (div1 * div2 * div3)) * mul2) << 4;
}

static int rtcl_838x_set_rate(int clk_idx, const struct rtcl_reg_set *reg)
{
	u32 ctrl0, ctrl1;
	unsigned long irqflags;

	ctrl0 = FIELD_PREP(RTL838X_PLL_CMU_CTRL0_DIVN2, reg->divn2) |
	        FIELD_PREP(RTL838X_PLL_CMU_CTRL0_NCODE_IN, reg->ncode_in);
	ctrl1 = FIELD_PREP(RTL838X_PLL_CMU_CTRL1_DIVN3_SEL, reg->selb) |
	        reg->selb ? RTL838X_PLL_CMU_CTRL1_DIVN2_SELB : 0;
	/*
	 * Runtime of this function (including locking)
	 * CPU: up to 14000 cycles / up to 56 us at 250 MHz (half default speed)
	 */
	spin_lock_irqsave(&rtcl_ccu->lock, irqflags);
	rtcl_838x_sram_set_rate(clk_idx, ctrl0, ctrl1);
	spin_unlock_irqrestore(&rtcl_ccu->lock, irqflags);

	return 0;
}

static int rtcl_839x_set_rate(int clk_idx, const struct rtcl_reg_set *reg)
{
	u32 ctrl0, ctrl1;
	unsigned long vpflags;
	unsigned long irqflags;

	ctrl0 = FIELD_PREP(RTL839X_PLL_CMU_CTRL0_DIVN2, reg->divn2) |
	        FIELD_PREP(RTL839X_PLL_CMU_CTRL0_NCODE_IN, reg->ncode_in);
	ctrl1 = FIELD_PREP(RTL839X_PLL_CMU_CTRL1_DIVN3_SEL, reg->selb) |
	        reg->selb ? RTL839X_PLL_CMU_CTRL1_DIVN2_SELB : 0;
	/*
	 * Runtime of this function (including locking)
	 * CPU: up to 31000 cycles / up to 89 us at 350 MHz (half default speed)
	 */
	spin_lock_irqsave(&rtcl_ccu->lock, irqflags);
	vpflags = dvpe();
	rtcl_839x_sram_set_rate(clk_idx, ctrl0, ctrl1);
	evpe(vpflags);
	spin_unlock_irqrestore(&rtcl_ccu->lock, irqflags);

	return 0;
}

static int rtcl_930x_set_rate(int clk_idx, const struct rtcl_reg_set *reg)
{
	u32 ctrl0, ctrl1;
	unsigned long vpflags;
	unsigned long irqflags;

	ctrl0 = FIELD_PREP(RTL930X_PLL_CMU_CTRL0_NCODE_IN, reg->ncode_in);
	ctrl1 = FIELD_PREP(RTL930X_PLL_CPU_MISC_CTRL_DIVN2_CPU, reg->divn2);
	/*
	 * Runtime of this function (including locking)
	 * CPU: up 19000 cycles / up to 48 us at 400 MHz (half default speed)
	 */
	spin_lock_irqsave(&rtcl_ccu->lock, irqflags);
	vpflags = dvpe();
	rtcl_930x_dram_set_rate(clk_idx, ctrl0, ctrl1);
	evpe(vpflags);
	spin_unlock_irqrestore(&rtcl_ccu->lock, irqflags);

	return 0;
}

static int rtcl_set_rate(struct clk_hw *hw, unsigned long rate, unsigned long parent_rate)
{
	int tab_idx;
	struct rtcl_clk *clk = rtcl_hw_to_clk(hw);
	const struct rtcl_rtab_set *rtab = &rtcl_rtab_set[rtcl_ccu->soc][clk->idx];
	const struct rtcl_round_set *round = &rtcl_round_set[rtcl_ccu->soc][clk->idx];

	if ((clk->idx != CLK_CPU) || (!rtcl_ccu->sram.vbase))
		return -EINVAL;

	/*
	 * Currently we do not know if SRAM is stable on these devices. Maybe
	 * someone changes memory in this region and does not care about proper
	 * allocation. So check if something might go wrong.
	 */
	if (unlikely(*rtcl_ccu->sram.pmark != RTL_SRAM_SET_PLL_RATE_CANARY)) {
		dev_err(&rtcl_ccu->pdev->dev, "SRAM code lost\n");
		return -EINVAL;
	}

	tab_idx = (rate - round->min) / round->step;
	if ((tab_idx < 0) || (tab_idx >= rtab->count) || (rtab->rset[tab_idx].rate != rate))
		return -EINVAL;

	switch (rtcl_ccu->soc) {
	case SOC_RTL838X:
		return rtcl_838x_set_rate(clk->idx, &rtab->rset[tab_idx]);
	case SOC_RTL839X:
		return rtcl_839x_set_rate(clk->idx, &rtab->rset[tab_idx]);
	case SOC_RTL930X:
		return rtcl_930x_set_rate(clk->idx, &rtab->rset[tab_idx]);
	}

	return -ENXIO;
}

static long rtcl_round_rate(struct clk_hw *hw, unsigned long rate, unsigned long *parent_rate)
{
	struct rtcl_clk *clk = rtcl_hw_to_clk(hw);
	unsigned long rrate = max(clk->min, min(clk->max, rate));
	const struct rtcl_round_set *round = &rtcl_round_set[rtcl_ccu->soc][clk->idx];

	rrate = ((rrate + (round->step >> 1)) / round->step) * round->step;
	rrate -= (rrate > clk->max) ? round->step : 0;
	rrate += (rrate < clk->min) ? round->step : 0;

	return rrate;
}

/*
 * Initialization functions to register the CCU and its clocks
 */

#define RTCL_SRAM_FUNC(SOC, PBASE, FN) ({				\
        rtcl_##SOC##_sram_##FN = ((void *)&rtcl_##SOC##_dram_##FN -	\
                                  (void *)&rtcl_##SOC##_dram_start) +	\
                                  (void *)PBASE; })

static const struct clk_ops rtcl_clk_ops = {
	.set_rate = rtcl_set_rate,
	.round_rate = rtcl_round_rate,
	.recalc_rate = rtcl_recalc_rate,
};

static int rtcl_ccu_create(struct device_node *np)
{
	int soc;

	if (of_device_is_compatible(np, "realtek,rtl838x-clock"))
		soc = SOC_RTL838X;
	else if (of_device_is_compatible(np, "realtek,rtl839x-clock"))
		soc = SOC_RTL839X;
	else if (of_device_is_compatible(np, "realtek,rtl930x-clock"))
		soc = SOC_RTL930X;
	else
		return -ENXIO;

	rtcl_ccu = kzalloc(sizeof(*rtcl_ccu), GFP_KERNEL);
	if (IS_ERR(rtcl_ccu))
		return -ENOMEM;

	rtcl_ccu->np = np;
	rtcl_ccu->soc = soc;
	spin_lock_init(&rtcl_ccu->lock);

	return 0;
}

int rtcl_register_clkhw(int clk_idx)
{
	int ret;
	struct rtcl_clk *rclk = &rtcl_ccu->clks[clk_idx];

	rclk->idx = clk_idx;
	rclk->hw.init = CLK_HW_INIT(rtcl_clk_info[clk_idx].name,
	                            rtcl_clk_info[clk_idx].parent_name[rtcl_ccu->soc],
	                            &rtcl_clk_ops,
	                            0);

	ret = of_clk_hw_register(rtcl_ccu->np, &rclk->hw);
	if (ret)
		return ret;

	clk_hw_register_clkdev(&rclk->hw, rtcl_clk_info[clk_idx].name, NULL);

	rclk->startup = clk_hw_get_rate(&rclk->hw);

	switch (clk_idx) {
	case CLK_CPU:
		rclk->min = rtcl_round_set[rtcl_ccu->soc][clk_idx].min;
		rclk->max = rtcl_round_set[rtcl_ccu->soc][clk_idx].max;
		break;
	default:
		/*
		 * TODO: This driver supports PLL reclocking and nothing else.
		 * Additional required steps for non CPU PLLs are missing.
		 * E.g. if we want to change memory clocks the right way we must
		 * adapt a lot of other settings. This includes MCR and DTRx
		 * timing registers (0xb80001000, 0xb8001008, ...) and a DLL
		 * reset so that hardware operates in the allowed limits. This
		 * is far too complex without official support. Avoid for now.
		 */
		rclk->min = rclk->max = rclk->startup;
		break;
	}

	return 0;
}

static struct clk_hw *rtcl_get_clkhw(struct of_phandle_args *clkspec, void *prv)
{
	unsigned int idx = clkspec->args[0];

	if (idx >= CLK_COUNT) {
		pr_err("%s: Invalid index %u\n", __func__, idx);
		return ERR_PTR(-EINVAL);
	}

	return &rtcl_ccu->clks[idx].hw;
}

static int rtcl_ccu_register_clocks(void)
{
	int clk_idx, ret;

        /*
         * TODO: Rewrite clock driver to properly implement the actual main
         * PLL's and their clocks. Keep these here for now to at least make
         * the clock-tree accurate.
         * The SRAM clk might actually come from the OCP clock.
         * Also missing are the SPI-[NOR|NAND] children.
         */
	if (rtcl_ccu->soc == SOC_RTL930X) {
	        struct clk *clk;

                clk = clk_register_fixed_factor(NULL, "ocp_clk", "ref_clk", 0, 1, 1);
                clk_register_clkdev(clk, "ocp_clk", NULL);
                clk = clk_register_fixed_factor(NULL, "mem_clk", "ref_clk", 0, 1, 1);
                clk_register_clkdev(clk, "mem_clk", NULL);
                clk = clk_register_fixed_factor(NULL, "sram_clk", "ref_clk", 0, 10, 1);
                clk_register_clkdev(clk, "sram_clk", NULL);
        }

	for (clk_idx = 0; clk_idx < CLK_COUNT; clk_idx++) {
		ret = rtcl_register_clkhw(clk_idx);
		if (ret) {
			pr_err("%s: Couldn't register %s clock\n",
				__func__, rtcl_clk_info[clk_idx].display_name);
			goto err_hw_unregister;
		}
	}

	ret = of_clk_add_hw_provider(rtcl_ccu->np, rtcl_get_clkhw, rtcl_ccu);
	if (ret) {
		pr_err("%s: Couldn't register clock provider of %s\n",
			__func__, of_node_full_name(rtcl_ccu->np));
		goto err_hw_unregister;
	}

	return 0;

err_hw_unregister:
	for (--clk_idx; clk_idx >= 0; --clk_idx)
		clk_hw_unregister(&rtcl_ccu->clks[clk_idx].hw);

	return ret;
}

int rtcl_init_sram(struct device *dev)
{
	struct gen_pool *sram_pool;
	phys_addr_t sram_pbase;
	unsigned long sram_vbase;
	struct device_node *node = dev->of_node;
	struct platform_device *pdev = NULL;
	void *dram_start;
	int dram_size;
	const char *wrn = ", rate setting disabled.\n";

	sram_pool = of_gen_pool_get(node, "pll-reclock-sram", 0);
	if (!sram_pool) {
		dev_warn(dev, "SRAM pool 'pll-reclock-sram' not in dt%s", wrn);
		return -ENXIO;
        }

	switch (rtcl_ccu->soc) {
	case SOC_RTL838X:
		dram_start = &rtcl_838x_dram_start;
		dram_size = rtcl_838x_dram_size;
		break;
	case SOC_RTL839X:
		dram_start = &rtcl_839x_dram_start;
		dram_size = rtcl_839x_dram_size;
		break;
	case SOC_RTL930X:
		dram_start = &rtcl_930x_dram_start;
		dram_size = rtcl_930x_dram_size;
		break;
	default:
		return -ENXIO;
	}

	sram_vbase = gen_pool_alloc(sram_pool, dram_size);
	if (!sram_vbase) {
		dev_warn(dev, "can not allocate SRAM%s", wrn);
		return -ENXIO;
	}

	sram_pbase = gen_pool_virt_to_phys(sram_pool, sram_vbase);
	memcpy((void *)sram_pbase, dram_start, dram_size);
	flush_icache_range((unsigned long)sram_pbase, (unsigned long)(sram_pbase + dram_size));

	switch (rtcl_ccu->soc) {
	case SOC_RTL838X:
		RTCL_SRAM_FUNC(838x, sram_pbase, set_rate);
		break;
	case SOC_RTL839X:
		RTCL_SRAM_FUNC(839x, sram_pbase, set_rate);
		break;
	case SOC_RTL930X:
		RTCL_SRAM_FUNC(930x, sram_pbase, set_rate);
		break;
	}

	rtcl_ccu->sram.pmark = (int *)((void *)sram_pbase + (dram_size - 4));
	rtcl_ccu->sram.vbase = sram_vbase;

	return 0;
}

void rtcl_ccu_log_early(void)
{
	char clkinfo[255], msg[255] = "rtl83xx-clk: initialized";

	for (int clk_idx = 0; clk_idx < CLK_COUNT; clk_idx++) {
		sprintf(clkinfo, ", %s %lu MHz", rtcl_clk_info[clk_idx].display_name,
			rtcl_ccu->clks[clk_idx].startup / 1000000);
		strcat(msg, clkinfo);
	}
	pr_info("%s\n", msg);
}

void rtcl_ccu_log_late(void)
{
	struct rtcl_clk *rclk;
	bool overclock = false;
	char clkinfo[80], msg[255] = "rate setting enabled";

	for (int clk_idx = 0; clk_idx < CLK_COUNT; clk_idx++) {
		rclk = &rtcl_ccu->clks[clk_idx];
		overclock |= rclk->max > rclk->startup;
		sprintf(clkinfo, ", %s %lu-%lu MHz", rtcl_clk_info[clk_idx].display_name,
			rclk->min / 1000000, rclk->max / 1000000);
		strcat(msg, clkinfo);
	}
	if (overclock)
		strcat(msg, ", OVERCLOCK AT OWN RISK");

	dev_info(&rtcl_ccu->pdev->dev, "%s\n", msg);
}

/*
 * Early registration: This module provides core startup clocks that are needed
 * for generic SOC init and for further builtin devices (e.g. UART). Register
 * asap via clock framework.
 */

static void __init rtcl_probe_early(struct device_node *np)
{
	if (rtcl_ccu_create(np))
		return;

	if (rtcl_ccu_register_clocks())
		kfree(rtcl_ccu);
	else
		rtcl_ccu_log_early();
}

CLK_OF_DECLARE_DRIVER(rtl838x_clk, "realtek,rtl838x-clock", rtcl_probe_early);
CLK_OF_DECLARE_DRIVER(rtl839x_clk, "realtek,rtl839x-clock", rtcl_probe_early);
CLK_OF_DECLARE_DRIVER(rtl930x_clk, "realtek,rtl930x-clock", rtcl_probe_early);

/*
 * Late registration: Finally register as normal platform driver. At this point
 * we can make use of other modules like SRAM.
 */

static const struct of_device_id rtcl_dt_ids[] = {
	{ .compatible = "realtek,otto-clock" },
	/* RTL838x */
	{ .compatible = "realtek,maple-clock" },
	{ .compatible = "realtek,rtl838x-clock" },
	{ .compatible = "realtek,rtl8380-clock" },
	{ .compatible = "realtek,rtl8381-clock" },
	{ .compatible = "realtek,rtl8382-clock" },
	/* RTL839x */
	{ .compatible = "realtek,cypress-clock" },
	{ .compatible = "realtek,rtl839x-clock" },
	{ .compatible = "realtek,rtl8390-clock" },
	{ .compatible = "realtek,rtl8391-clock" },
	{ .compatible = "realtek,rtl8392-clock" },
	{ .compatible = "realtek,rtl8393-clock" },
	/* RTL930x */
	{ .compatible = "realtek,longan-clock" },
	{ .compatible = "realtek,rtl930x-clock" },
	{ .compatible = "realtek,rtl9301-clock" },
	{ .compatible = "realtek,rtl9302-clock" },
	{ .compatible = "realtek,rtl9303-clock" },
	/* RTL931x */
	{ /* sentinel */ }
};

static int rtcl_probe_late(struct platform_device *pdev)
{
	int ret;

	if (!rtcl_ccu) {
		dev_err(&pdev->dev, "early initialization not run");
		return -ENXIO;
	}
	rtcl_ccu->pdev = pdev;
	ret = rtcl_init_sram(&pdev->dev);
	if (ret)
		return ret;

	rtcl_ccu_log_late();

	return 0;
}

static struct platform_driver rtcl_platform_driver = {
	.driver = {
		.name = "rtl83xx-clk",
		.of_match_table = rtcl_dt_ids,
	},
	.probe = rtcl_probe_late,
};

static int __init rtcl_init_subsys(void)
{
	return platform_driver_register(&rtcl_platform_driver);
}

/*
 * The driver does not know when SRAM module has finally loaded. With an
 * arch_initcall() we might overtake SRAM initialization. Be polite and give the
 * system a little more time.
 */

subsys_initcall(rtcl_init_subsys);
