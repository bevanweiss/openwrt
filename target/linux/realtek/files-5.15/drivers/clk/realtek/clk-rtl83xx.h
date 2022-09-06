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

/* RTL930X */
#define RTL930X_PLL_GLB_CTRL0_REG               (0xe200)
/* Reserved                                             31 */
#define RTL930X_PLL_GLB_CTRL0_PLL_DBG_OUT               GENMASK(30, 20)
#define RTL930X_PLL_GLB_CTRL0_CPU_CLK_SEL               BIT(19)
#define RTL930X_PLL_GLB_CTRL0_NOR_CLK_SEL               BIT(18)
#define RTL930X_PLL_GLB_CTRL0_LXB_CLK_SEL               BIT(17)
#define RTL930X_PLL_GLB_CTRL0_SW_PLL_READY              BIT(16)
#define RTL930X_PLL_GLB_CTRL0_CPU_PLL_READY             BIT(15)
#define RTL930X_PLL_GLB_CTRL0_125M_PLL_READY            BIT(14)
#define RTL930X_PLL_GLB_CTRL0_BYPASS_ALE_PLL            BIT(13)
#define RTL930X_PLL_GLB_CTRL0_BYPASS_NOR_PLL            BIT(12)
#define RTL930X_PLL_GLB_CTRL0_BYPASS_LXB_PLL            BIT(11)
#define RTL930X_PLL_GLB_CTRL0_BYPASS_CPU_PLL            BIT(10)
#define RTL930X_PLL_GLB_CTRL0_BYPASS_125M_PLL           BIT(9)
#define RTL930X_PLL_GLB_CTRL0_BYPASS_SRAM_PLL           BIT(8)
#define RTL930X_PLL_GLB_CTRL0_EN_DDR_PLL                BIT(7)
#define RTL930X_PLL_GLB_CTRL0_EN_NOR_PLL                BIT(6)
#define RTL930X_PLL_GLB_CTRL0_EN_NAND_PLL               BIT(5)
#define RTL930X_PLL_GLB_CTRL0_EN_USB_PLL                BIT(4)
#define RTL930X_PLL_GLB_CTRL0_EN_LXB_PLL                BIT(3)
#define RTL930X_PLL_GLB_CTRL0_EN_CPU_PLL                BIT(2)
#define RTL930X_PLL_GLB_CTRL0_EN_125M_PLL               BIT(1)
#define RTL930X_PLL_GLB_CTRL0_EN_SRAM_PLL               BIT(0)

#define RTL930X_PLL_GLB_CTRL1_REG               (0xe204)
/* Reserved                                             31 - 11 */
#define RTL930X_PLL_GLB_CTRL1_DLL_PRE_DIVN              GENMASK(10, 9)
#define RTL930X_PLL_GLB_CTRL1_EN_PLL_MON                GENMASK(8, 6)
#define RTL930X_PLL_GLB_CTRL1_CMU_DLY_EN_CPU            BIT(5)
#define RTL930X_PLL_GLB_CTRL1_CMU_DLY_EN_SW             BIT(4)
#define RTL930X_PLL_GLB_CTRL1_CMU_DLY_EN_125M           BIT(3)
#define RTL930X_PLL_GLB_CTRL1_FLD_DSEL_CPU              BIT(2)
#define RTL930X_PLL_GLB_CTRL1_FLD_DSEL_SW               BIT(1)
#define RTL930X_PLL_GLB_CTRL1_FLD_DSEL_125M             BIT(0)

#define RTL930X_PLL_CPU_CTRL0_REG               (0xe208)
/* Reserved                                             31 - 28 */
#define RTL930X_PLL_CPU_CTRL0_TEST_EN                   BIT(27)
#define RTL930X_PLL_CPU_CTRL0_DIVN3                     GENMASK(26, 25)
#define RTL930X_PLL_CPU_CTRL0_DIVN3_DIV(reg) \
        ((reg) + 1)
/* Rest of the registers are identical, see CMU_CTRL0 below */

#define RTL930X_PLL_CPU_CTRL1_REG               (0xe20c)
#define RTL930X_PLL_CPU_CTRL1_SSC_EN                    BIT(31)
#define RTL930X_PLL_CPU_CTRL1_STEP_IN                   GENMASK(30, 18)
#define RTL930X_PLL_CPU_CTRL1_TBASE_IN                  GENMASK(17, 6)
#define RTL930X_PLL_CPU_CTRL1_SSC_ORDER                 BIT(5)
#define RTL930X_PLL_CPU_CTRL1_TIME2_RST_WIDTH           GENMASK(4, 3)
#define RTL930X_PLL_CPU_CTRL1_TIME0_CK                  GENMASK(2, 0)

#define RTL930X_PLL_CPU_MISC_CTRL_REG           (0xe210)
/* Reserved                                             31 */
#define RTL930X_PLL_CPU_MISC_CTRL_EN                    BIT(30)
#define RTL930X_PLL_CPU_MISC_CTRL_CLKRDY                GENMASK(29, 28)
#define RTL930X_PLL_CPU_MISC_CTRL_BIG_KVCO              BIT(27)
#define RTL930X_PLL_CPU_MISC_CTRL_LPF_RS                GENMASK(26, 24)
#define RTL930X_PLL_CPU_MISC_CTRL_EN_CENTER_IN          BIT(23)
#define RTL930X_PLL_CPU_MISC_CTRL_EN_WD                 BIT(22)
#define RTL930X_PLL_CPU_MISC_CTRL_PI_I_SEL              GENMASK(21, 20)
#define RTL930X_PLL_CPU_MISC_CTRL_SEL_CP_I              GENMASK(19, 16)
#define RTL930X_PLL_CPU_MISC_CTRL_SEL_CCO               BIT(16)
#define RTL930X_PLL_CPU_MISC_CTRL_LDO_SEL               GENMASK(15, 13)
#define RTL930X_PLL_CPU_MISC_CTRL_LPF_CP                BIT(11)
#define RTL930X_PLL_CPU_MISC_CTRL_CP_NEW_EN             BIT(10)
#define RTL930X_PLL_CPU_MISC_CTRL_LDO_EN                BIT(9)
#define RTL930X_PLL_CPU_MISC_CTRL_VC_DLY                BIT(8)
#define RTL930X_PLL_CPU_MISC_CTRL_EN_CKOOBS             BIT(7)
#define RTL930X_PLL_CPU_MISC_CTRL_DIVN2_CPU             GENMASK(6, 4)
#define RTL930X_PLL_CPU_MISC_CTRL_DIVN2_CPU_DIV(reg) \
        ((reg) + 2)
#define RTL930X_PLL_CPU_MISC_CTRL_DIVN2_DLL             GENMASK(3, 1)
#define RTL930X_PLL_CPU_MISC_CTRL_EN_DLL                BIT(0)

#define RTL930X_PLL_SW_CTRL0_REG                (0xe214)
/* Reserved                                             31 - 26 */
#define RTL930X_PLL_SW_CTRL0_TEST_EN                    BIT(25)
/* Rest of the registers are identical, see CMU_CTRL0 below */

#define RTL930X_PLL_SW_CTRL1_REG                (0xe218)
/* Reserved                                             31 - 26 */
/* Rest of the registers are identical, see CMU_CTRL1 below */

#define RTL930X_PLL_SW_MISC_CTRL_REG            (0xe21c)
/* Reserved                                             31 - 29 */
#define RTL930X_PLL_SW_MISC_SSC_ORDER                   BIT(28)
#define RTL930X_PLL_SW_MISC_TIME2_RST_WIDTH             GENMASK(27, 26)
#define RTL930X_PLL_SW_MISC_TIME0_CK                    GENMASK(25, 24)
#define RTL930X_PLL_SW_MISC_CLKRDY                      GENMASK(23, 21)
#define RLT930X_PLL_SW_MISC_BIG_KVCO                    BIT(20)
#define RLT930X_PLL_SW_MISC_LPF_RS                      GENMASK(19, 17)
#define RLT930X_PLL_SW_MISC_CENTER_IN_EN                BIT(16)
#define RLT930X_PLL_SW_MISC_WD_EN                       BIT(15)
#define RLT930X_PLL_SW_MISC_PI_I_SEL                    GENMASK(14, 13)
#define RLT930X_PLL_SW_MISC_CPI_I_SEL                   GENMASK(12, 9)
#define RLT930X_PLL_SW_MISC_SEL_CCO                     BIT(8)
#define RLT930X_PLL_SW_MISC_LDO_SEL                     GENMASK(7, 5)
#define RLT930X_PLL_SW_MISC_LPF_CP                      BIT(4)
#define RLT930X_PLL_SW_MISC_CP_NEW_EN                   BIT(3)
#define RLT930X_PLL_SW_MISC_LDO_EN                      BIT(2)
#define RLT930X_PLL_SW_MISC_VC_DLY                      BIT(1)
#define RLT930X_PLL_SW_MISC_CKOOBS_EN                   BIT(0)

#define RTL930X_PLL_SW_DIV_CTRL_REG             (0xe220)
/* Reserved                                             31 - 18 */
#define RTL930X_PLL_SW_DIV_CTRL_EN                      BIT(17)
#define RTL930X_PLL_SW_DIV_CTRL_DIVN2_SPI_NOR           GENMASK(16, 13)
#define RTL930X_PLL_SW_DIV_CTRL_DIVN2_LXB_NAND_USB      GENMASK(12, 9)
#define RTL930X_PLL_SW_DIV_CTRL_DIVN2_LXB_NAND_USB_DIV(reg) \
        (((reg) + 2) * 2)
#define RTL930X_PLL_SW_DIV_CTRL_DIVN2_SWCORE            GENMASK(8, 5)
#define RTL930X_PLL_SW_DIV_CTRL_DIVN2_GPHY_DBG          GENMASK(4, 1)
#define RTL930X_PLL_SW_DIV_CTRL_GPHY_DBG_EN             BIT(0)

#define RTL930X_PLL_125M_CTRL0_REG              (0xe224)
#define RLT930X_PLL_125M_CTRL0_TEST_EN                  BIT(25)
/* Rest of the registers are identical, see CMU_CTRL0 below */

#define RTL930X_PLL_125M_CTRL1_REG              (0xe228)
#define RTL930X_PLL_125M_CTRL1_DIVN2                    GENMASK(29, 26)
/* Rest of the registers are identical, see CMU_CTRL1 below */

#define RTL930X_PLL_125M_MISC_REG               (0xe22c)

/* Shared CMU_CTRL0 registers */
#define RLT930X_PLL_CMU_CTRL0_FCODE_IN                  GENMASK(24, 12)
#define RTL930X_PLL_CMU_CTRL0_NCODE_IN                  GENMASK(11, 4)
#define RTL930X_PLL_CMU_CTRL0_NCODE_IN_CODE(reg) \
        (((reg) + 2) * 2)
#define RTL930X_PLL_CMU_CTRL0_BYPASS_PI                 BIT(3)
#define RTL930X_PLL_CMU_CTRL0_SEL_DIV4                  BIT(2)
#define RTL930X_PLL_CMU_CTRL0_SEL_DIV4_DIV(reg) \
        ((reg) ? 4 : 1)
#define RTL930X_PLL_CMU_CTRL0_SEL_PREDIV                GENMASK(1, 0)
#define RTL930X_PLL_CMU_CTRL0_SEL_PREDIV_DIV(reg) \
        (1 << (reg))

/* Shared CMU_CTRL1 registers */
#define RTL930X_PLL_CMU_CTRL1_SSC_EN                    BIT(25)
#define RTL930X_PLL_CMU_CTRL1_STEP_IN                   GENMASK(24, 12)
#define RTL930X_PLL_CMU_CTRL1_TBASE_IN                  GENMASK(11, 0)

#define RTL_PLL_CTRL0_CMU_SEL_PREDIV(v)		(((v) >> 0) & 0x3)
#define RTL_PLL_CTRL0_CMU_SEL_DIV4(v)		(((v) >> 2) & 0x1)
#define RTL_PLL_CTRL0_CMU_NCODE_IN(v)		(((v) >> 4) & 0xff)
#define RTL_PLL_CTRL0_CMU_DIVN2(v)		(((v) >> 12) & 0xff)

/* Other stuff */
#define RTL_SRAM_SET_PLL_RATE_CANARY    (0x5eaf00d5)

/* SoC Peripherial address space */
/* TODO: This should be first extracted from asm, later removed/replaced */
#define RTL930X_MODEL_NAME_INFO_REG             (0x0004)
#define RTL930X_MODEL_NAME_INFO_VID                     GENMASK(3, 0)

#define RTL930X_SYS_STATUS_REG                  (0x0044)
/* Reserved                                             31 - 11 */
#define RTL930X_SYS_STATUS_GNT_DLY                      GENMASK(10, 8)
/* Reserved                                             7 - 5 */
#define RTL930X_SYS_STATUS_CLK_SEL_LX                   BIT(4)
#define RTL930X_SYS_STATUS_CLK_SEL_OCP1                 BIT(3)
#define RTL930X_SYS_STATUS_CLK_SEL_OCP0                 BIT(2)
#define RLT930X_SYS_STATUS_RDY_FOR_PATCH                BIT(1)
#define RTL930X_SYS_STATUS_SOC_INIT_RDY                 BIT(0)
