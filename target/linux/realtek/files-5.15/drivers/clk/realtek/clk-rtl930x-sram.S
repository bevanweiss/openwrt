/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Realtek RTL930X SRAM clock setters
 * Copyright (C) 2022 Markus Stockhausen <markus.stockhausen@gmx.de>
 */

#include <asm/mipsregs.h>
#include <dt-bindings/clock/rtl83xx-clk.h>

#include "clk-rtl83xx.h"

#define RTL930X_SWITCHCORE_BASE_ADDR    (0xbb000000) /* KSEG1(0x1b000000) */
#define RTL930X_SOC_BASE_ADDR           (0xb8000000) /* KSEG1(0x18000000) */
#define RTL930X_SRAM_BASE_ADDR          (0xbf000000) /* KSEG1(0x1f000000) */

#define rSW	$t0
#define rSOC	$t1
#define rMSK	$t2
#define rREG	$t3
#define rTMP1	$t4
#define rTMP2	$t5
#define rCP0	$t6
#define rONE	$t7

.set	noreorder

.globl	rtcl_930x_dram_start
rtcl_930x_dram_start:

/*
 * Functions start here and should avoid access to normal memory. REMARK! Do not
 * forget about stack pointer and dirty caches that might interfere.
 */

.globl	rtcl_930x_dram_set_rate
.ent	rtcl_930x_dram_set_rate
rtcl_930x_dram_set_rate:

	/* disable MIPS 34K branch and return prediction */
	mfc0	rCP0, CP0_CONFIG, 7
	ori	rTMP1, rTMP1, 0xc
	mtc0	rTMP1, CP0_CONFIG, 7

	li	rONE, -1

	/* write & read SRAM staging to clear ongoing traffic */
	li	rMSK, RTL930X_SRAM_BASE_ADDR
	lw	rTMP1, 0(rMSK)
	lw	rTMP2, 4(rMSK)
	sw	rTMP1, 0(rMSK)
	sw	rTMP2, 4(rMSK)
	lw	rTMP1, 0(rMSK)
	lw	rTMP2, 4(rMSK)

	li	rSW, RTL930X_SWITCHCORE_BASE_ADDR
	li	rSOC, RTL930X_SOC_BASE_ADDR

	/* switch CPU to LXB clock */
	ori	rREG, rSOC, RTL930X_SYS_STATUS_REG
	lw	rTMP1, 0(rREG)
	ins	rTMP1, $0, 2, 1          /* !RTL930X_SYS_STATUS_CLK_SEL_OCP0 */
	sw	rTMP1, 0(rREG)

	/* set new PLL values */
	ori	rREG, rSW, RTL930X_PLL_CPU_CTRL0_REG
	lw	rTMP1, 0(rREG)
	li	rMSK, 0xfffff00f          /* !RTL930X_PLL_CMU_CTRL0_NCODE_IN */
	and	rTMP1, rTMP1, rMSK
	or	rTMP1, rTMP1, $a1                          /* argv[1]: ctrl0 */
	sw	rTMP1, 0(rREG)
	lw	rTMP1, 8(rREG)              /* RTL930X_PLL_CPU_MISC_CTRL_REG */
	li	rMSK, 0xffffff8f     /* !RTL930X_PLL_CPU_MISC_CTRL_DIVN2_CPU */
	and	rTMP1, rTMP1, rMSK
	or	rTMP1, rTMP1, $a2                          /* argv[2]: ctrl1 */
	sw	rTMP1, 8(rREG)

	/* use trigger depending on model */
	ori	rREG, rSW, RTL930X_MODEL_NAME_INFO_REG
	lw	rTMP1, 0(rREG)
	ext	rTMP2, rTMP1, 4, 1   /* RTL930X_MODEL_NAME_INFO_VID & BIT(4) */
	beqz	rTMP2, trigger_glb_ctrl
	li	rTMP2, 3                           /* Note branch delay slot */
	ext	rTMP1, rTMP1, 0, 4            /* RTL930X_MODEL_NAME_INFO_VID */
	bge	rTMP1, rTMP2, trigger_glb_ctrl
	nop                                        /* Note branch delay slot */
trigger_sw_div_ctrl:
	ori	rREG, rSW, RTL930X_PLL_SW_DIV_CTRL_REG
	lw	rTMP1, 0(rREG)
	ins	rTMP1, $0, 17, 1               /* RTL930X_PLL_SW_DIV_CTRL_EN */
	sw	rTMP1, 0(rREG)
	ins	rTMP1, rONE, 17, 1             /* RTL930X_PLL_SW_DIV_CTRL_EN */
	sw	rTMP1, 0(rREG)
	b	wait_pll_ready
	nop
trigger_glb_ctrl:
	ori	rREG, rSW, RTL930X_PLL_GLB_CTRL0_REG
	lw	rTMP1, 0(rREG)
	ins	rTMP1, $0, 30, 1    /* RTL930X_PLL_GLB_CTRL0_PLL_DBG_OUT[10] */
	sw	rTMP1, 0(rREG)
	ins	rTMP1, rONE, 30, 1  /* RTL930X_PLL_GLB_CTRL0_PLL_DBG_OUT[10] */
	sw	rTMP1, 0(rREG)
	ori	rREG, rSW, RTL930X_PLL_GLB_CTRL0_REG
wait_pll_ready:
	lw	rTMP1, 0(rREG)
	ext	rTMP2, rTMP1, 15, 1   /* RTL930X_PLL_GLB_CTRL0_CPU_PLL_READY */
	beqz	rTMP2, wait_pll_ready
	nop                                        /* Note branch delay slot */

	/* switch CPU to PLL clock */
	ori	rREG, rSOC, RTL930X_SYS_STATUS_REG
	lw	rTMP1, 0(rREG)
	ins	rTMP1, rONE, 2, 1         /* RTL930X_SYS_STATUS_CLK_SEL_OCP0 */
	sw	rTMP1, 0(rREG)

	/* stabilize clock */
	li	rTMP1, 5000
wait_clock:
	bnez	rTMP1, wait_clock
	addiu	rTMP1, rTMP1, -1                   /* Note branch delay slot */

	/* restore branch prediction */
	mtc0	rCP0, CP0_CONFIG, 7
	jr	$ra
	nop                                        /* Note branch delay slot */

.end	rtcl_930x_dram_set_rate

/* SRAM Corruption/Correctness detection canary. Do not delete. */
	.word RTL_SRAM_SET_PLL_RATE_CANARY
.globl	rtcl_930x_dram_size
rtcl_930x_dram_size:
	.word .-rtcl_930x_dram_start
