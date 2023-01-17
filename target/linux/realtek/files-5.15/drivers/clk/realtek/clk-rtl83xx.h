/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Realtek RTL83XX clock headers
 * Copyright (C) 2022 Markus Stockhausen <markus.stockhausen@gmx.de>
 */

#ifdef __ASSEMBLY__
/* Allow us to include this header in our assembly files */
#define BIT(nr) (1 << (nr))
#else
#include <linux/bitops.h>

static const int rtl838x_pll_cmu_ctrl1_divn3_lut[] = {
        2, 3, 4, 6,
};

static const int rtl839x_pll_cmu_ctrl1_divn3_lut[] = {
        2, 3, 4, 6,
};
#endif

/* Switch registers (e.g. PLL) */
/* RTL838X */
#define RTL838X_PLL_GLB_CTRL_REG                (0x0fc0)
/* Reserved                                             31 - 15 */
#define RTL838X_PLL_GLB_CTRL_MEM_CLK_M_DIS              BIT(14)
#define RTL838X_PLL_GLB_CTRL_MEM_CHIP_CLK_INI           BIT(13)
#define RTL838X_PLL_GLB_CTRL_CPU_PLL_SC_MUX             BIT(12)
#define RTL838X_PLL_GLB_CTRL_SW_PLL_READY               BIT(11)
#define RTL838X_PLL_GLB_CTRL_MEM_PLL_READY              BIT(10)
#define RTL838X_PLL_GLB_CTRL_LXB_PLL_READY              BIT(9)
#define RTL838X_PLL_GLB_CTRL_CPU_PLL_READY              BIT(8)
#define RTL838X_PLL_GLB_CTRL_BYPASS_IF_PLL              BIT(7)
#define RTL838X_PLL_GLB_CTRL_BYPASS_SW_PLL              BIT(6)
#define RTL838X_PLL_GLB_CTRL_BYPASS_MEM_PLL             BIT(5)
#define RTL838X_PLL_GLB_CTRL_BYPASS_LXB_PLL             BIT(4)
#define RTL838X_PLL_GLB_CTRL_BYPASS_CPU_PLL             BIT(3)
#define RTL838X_PLL_GLB_CTRL_EN_MEM_PLL                 BIT(2)
#define RTL838X_PLL_GLB_CTRL_EN_LXB_PLL                 BIT(1)
#define RTL838X_PLL_GLB_CTRL_EN_CPU_PLL                 BIT(0)

#define RTL838X_PLL_CPU_CTRL0_REG               (0x0fc4)
#define RTL838X_PLL_CPU_CTRL1_REG               (0x0fc8)
#define RTL838X_PLL_CPU_MISC_CTRL_REG           (0x0fcc)
#define RTL838X_PLL_LXB_CTRL0_REG               (0x0fd0)
#define RTL838X_PLL_LXB_CTRL1_REG               (0x0fd4)
#define RTL838X_PLL_LXB_MISC_CTRL_REG           (0x0fd8)
#define RTL838X_PLL_MEM_CTRL0_REG               (0x0fdc)
#define RTL838X_PLL_MEM_CTRL1_REG               (0x0fe0)
#define RTL838X_PLL_MEM_MISC_CTRL_REG           (0x0fe4)
#define RTL838X_PLL_SW_CTRL0_REG                (0x0fe8)
#define RTL838X_PLL_SW_CTRL1_REG                (0x0fec)
#define RTL838X_PLL_SW_MISC_CTRL_REG            (0x0ff0)

/* Shared CMU_CTRL0 registers */
#define RTL838X_PLL_CMU_CTRL0_FCODE_IN                  GENMASK(31, 20)
#define RTL838X_PLL_CMU_CTRL0_DIVN2                     GENMASK(19, 12)
#define RTL838X_PLL_CMU_CTRL0_DIVN2_DIV(reg) \
        ((reg) + 4)
#define RTL838X_PLL_CMU_CTRL0_NCODE_IN                  GENMASK(11, 4)
#define RTL838X_PLL_CMU_CTRL0_NCODE_IN_CODE(reg) \
        ((reg) + 4)
#define RTL838X_PLL_CMU_CTRL0_BYPASS_PI                 BIT(3)
#define RTL838X_PLL_CMU_CTRL0_SEL_DIV4                  BIT(2)
#define RTL838X_PLL_CMU_CTRL0_SEL_DIV4_DIV(reg) \
        ((reg) ? 4 : 1)
#define RTL838X_PLL_CMU_CTRL0_SEL_PREDIV                GENMASK(1, 0)
#define RTL838X_PLL_CMU_CTRL0_SEL_PREDIV_DIV(reg) \
        (1 << (reg))

/* Shared CMU_CTRL1 registers */
/* Reserved                                             31 - 29 */
#define RTL838X_PLL_CMU_CTRL1_DIVN3_SEL                 GENMASK(28, 27)
#define RTL838X_PLL_CMU_CTRL1_DIVN3_SEL_DIV(reg) \
        (rtl838x_pll_cmu_ctrl1_divn3_lut[(reg)])
#define RTL838X_PLL_CMU_CTRL1_DIVN2_SELB                BIT(26)
#define RTL838X_PLL_CMU_CTRL1_SSC_EN                    BIT(25)
#define RTL838X_PLL_CMU_CTRL1_STEP_IN                   GENMASK(24, 12)
#define RTL838X_PLL_CMU_CTRL1_TBASE_IN                  GENMASK(12, 0)

#define RTL838X_PLL_CMU_MISC_SSC_ORDER                  BIT(31)
#define RTL838X_PLL_CMU_MISC_TIME2_RST_WIDTH            GENMASK(29, 28)
#define RTL838X_PLL_CMU_MISC_TIME0_CK                   GENMASK(27, 25)
#define RTL838X_PLL_CMU_MISC_CLK_RDY                    GENMASK(24, 23)
#define RTL838X_PLL_CMU_MISC_BIG_KVCO                   BIT(23)
#define RTL838X_PLL_CMU_MISC_LPF_RS                     GENMASK(22, 20)
#define RTL838X_PLL_CMU_MISC_CENTER_IN_EN               BIT(19)
#define RTL838X_PLL_CMU_MISC_WD_EN                      BIT(18)
/* Reserved                                             17 */
#define RTL838X_PLL_CMU_MISC_PI_I_SEL                   GENMASK(16, 14)
#define RTL838X_PLL_CMU_MISC_CPI_I_SEL                  GENMASK(13, 10)
#define RTL838X_PLL_CMU_MISC_CCO_SEL                    BIT(9)
#define RTL838X_PLL_CMU_MISC_LDO_SEL                    GENMASK(8, 6)
#define RTL838X_PLL_CMU_MISC_LPF_CP                     BIT(5)
#define RTL838X_PLL_CMU_MISC_CP_NEW_EN                  BIT(4)
#define RTL838X_PLL_CMU_MISC_LDO_EN                     BIT(3)
#define RTL838X_PLL_CMU_MISC_VC_DLY                     BIT(2)
#define RTL838X_PLL_CMU_MISC_CKOOBS_EN                  BIT(1)
#define RTL838X_PLL_CMU_MISC_TEST_EN                    BIT(0)

/* RTL839X */
#define RTL839X_PLL_GLB_CTRL_REG                (0x0024)
/* Reserved                                             31 - 15 */
#define RTL839X_PLL_GLB_CTRL_IBIAS_FILTER               BIT(14)
#define RTL839X_PLL_GLB_CTRL_LXB_CLKSEL                 BIT(13)
#define RTL839X_PLL_GLB_CTRL_MEM_CLKSEL                 BIT(12)
#define RTL839X_PLL_GLB_CTRL_CPU_CLKSEL                 BIT(11)
#define RTL839X_PLL_GLB_CTRL_EN_PLL_MON                 BIT(10)
#define RTL839X_PLL_GLB_CTRL_SEL_PLL_MON                GENMASK(9, 8)
#define RTL839X_PLL_GLB_CTRL_BYPASS_IF_PLL              BIT(7)
#define RTL839X_PLL_GLB_CTRL_BYPASS_SW_PLL              BIT(6)
#define RTL839X_PLL_GLB_CTRL_BYPASS_MEM_PLL             BIT(5)
#define RTL839X_PLL_GLB_CTRL_BYPASS_LXB_PLL             BIT(4)
#define RTL839X_PLL_GLB_CTRL_BYPASS_CPU_PLL             BIT(3)
#define RTL839X_PLL_GLB_CTRL_EN_MEM_PLL                 BIT(2)
#define RTL839X_PLL_GLB_CTRL_EN_LXB_PLL                 BIT(1)
#define RTL839X_PLL_GLB_CTRL_EN_CPU_PLL                 BIT(0)

#define RTL839X_PLL_CPU_CTRL0_REG               (0x0028)
#define RTL839X_PLL_CPU_CTRL1_REG               (0x002c)
#define RTL839X_PLL_CPU_MISC_CTRL_REG           (0x0034)
#define RTL839X_PLL_LXB_CTRL0_REG               (0x0038)
#define RTL839X_PLL_LXB_CTRL1_REG               (0x003c)
#define RTL839X_PLL_LXB_MISC_CTRL_REG           (0x0044)
#define RTL839X_PLL_MEM_CTRL0_REG               (0x0048)
#define RTL839X_PLL_MEM_CTRL1_REG               (0x004c)
#define RTL839X_PLL_MEM_MISC_CTRL_REG           (0x0054)
#define RTL839X_PLL_SW_CTRL_REG                 (0x0058)
#define RTL839X_PLL_SW_MISC_CTRL_REG            (0x005c)

/* Shared CMU_CTRL0 registers */
#define RTL839X_PLL_CMU_CTRL0_FCODE_IN                  GENMASK(31, 20)
#define RTL839X_PLL_CMU_CTRL0_DIVN2                     GENMASK(19, 12)
#define RTL839X_PLL_CMU_CTRL0_DIVN2_DIV(reg) \
        ((reg) + 4)
#define RTL839X_PLL_CMU_CTRL0_NCODE_IN                  GENMASK(11, 4)
#define RTL839X_PLL_CMU_CTRL0_NCODE_IN_CODE(reg) \
        ((reg) + 4)
#define RTL839X_PLL_CMU_CTRL0_BYPASS_PI                 BIT(3)
#define RTL839X_PLL_CMU_CTRL0_SEL_DIV4                  BIT(2)
#define RTL839X_PLL_CMU_CTRL0_SEL_DIV4_DIV(reg) \
        ((reg) ? 4 : 1)
#define RTL839X_PLL_CMU_CTRL0_SEL_PREDIV                GENMASK(1, 0)
#define RTL839X_PLL_CMU_CTRL0_SEL_PREDIV_DIV(reg) \
        (1 << (reg))

/* Shared CMU_CTRL1 registers */
/* Reserved                                             31 - 3 */
#define RTL839X_PLL_CMU_CTRL1_DIVN2_SELB                BIT(2)
#define RTL839X_PLL_CMU_CTRL1_DIVN3_SEL                 GENMASK(1, 0)
#define RTL839X_PLL_CMU_CTRL1_DIVN3_SEL_DIV(reg) \
        (rtl839x_pll_cmu_ctrl1_divn3_lut[(reg)])


#define RTL_PLL_CTRL0_CMU_SEL_PREDIV(v)		(((v) >> 0) & 0x3)
#define RTL_PLL_CTRL0_CMU_SEL_DIV4(v)		(((v) >> 2) & 0x1)
#define RTL_PLL_CTRL0_CMU_NCODE_IN(v)		(((v) >> 4) & 0xff)
#define RTL_PLL_CTRL0_CMU_DIVN2(v)		(((v) >> 12) & 0xff)

/* Other stuff */
#define RTL_SRAM_SET_PLL_RATE_CANARY    (0x5eaf00d5)
