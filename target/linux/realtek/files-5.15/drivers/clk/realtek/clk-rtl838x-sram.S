/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Realtek RTL838X SRAM clock setters
 * Copyright (C) 2022 Markus Stockhausen <markus.stockhausen@gmx.de>
 */

#include <dt-bindings/clock/rtl83xx-clk.h>

#include "clk-rtl83xx.h"

#define RTL838X_SWITCHCORE_BASE_ADDR    (0xbb000000) /* KSEG1(0x1b000000) */
#define RTL838X_SRAM_BASE               (0x9f000000) /* KSEG0(0x1f000000) */

#define rGLB	$t0
#define rCTR	$t1
#define rMSK	$t2
#define rSLP	$t3
#define rTMP	$t4
#define rSEL	$t5

.set	noreorder

.globl	rtcl_838x_dram_start
rtcl_838x_dram_start:

/*
 * Functions start here and should avoid access to normal memory. REMARK! Do not forget about
 * stack pointer and dirty caches that might interfere.
 */

.globl	rtcl_838x_dram_set_rate
.ent	rtcl_838x_dram_set_rate
rtcl_838x_dram_set_rate:

	li	rCTR, RTL838X_SWITCHCORE_BASE_ADDR
	addiu	rGLB, rCTR, RTL838X_PLL_GLB_CTRL_REG
	ori	rTMP, $0, CLK_CPU
	beq	$a0, rTMP, pre_cpu
	ori	rTMP, $0, CLK_MEM                  /* Note branch delay slot */
	beq	$a0, rTMP, pre_mem
	nop                                        /* Note branch delay slot */
pre_lxb:
	ori	rSLP, $0, RTL838X_PLL_GLB_CTRL_LXB_PLL_READY
	addiu	rCTR, rCTR, RTL838X_PLL_LXB_CTRL0_REG
	b	main_set
	ori	rMSK, $0, RTL838X_PLL_GLB_CTRL_EN_LXB_PLL /* Note branch delay slot */
pre_mem:
	/* simple 64K data cache flush to avoid unexpected memory access */
	li	rMSK, RTL838X_SRAM_BASE
	li	rTMP, 2048
pre_flush:
	lw	$0, 0(rMSK)
	addiu	rMSK, rMSK, 32
	addiu	rTMP, rTMP, -1
	bne	rTMP, $0, pre_flush
	lw	$0, -4(rMSK)                       /* Note branch delay slot */

	ori	rSLP, $0, RTL838X_PLL_GLB_CTRL_MEM_PLL_READY
	addiu	rCTR, rCTR, RTL838X_PLL_MEM_CTRL0_REG
	b	main_set
	ori	rMSK, $0, RTL838X_PLL_GLB_CTRL_EN_MEM_PLL /* Note branch delay slot */
pre_cpu:
	/* switch CPU to LXB clock */
	ori	rMSK, $0, RTL838X_PLL_GLB_CTRL_CPU_PLL_SC_MUX
	nor	rMSK, rMSK, $0
	lw	rTMP, 0(rGLB)
	and	rTMP, rTMP, rMSK
	sw	rTMP, 0(rGLB)

	ori	rSLP, $0, RTL838X_PLL_GLB_CTRL_CPU_PLL_READY
	addiu	rCTR, rCTR, RTL838X_PLL_CPU_CTRL0_REG
	ori	rMSK, $0, RTL838X_PLL_GLB_CTRL_EN_CPU_PLL
main_set:
	/* disable PLL */
	nor	rMSK, rMSK, 0
	lw	rTMP, 0(rGLB)
	and	rTMP, rTMP, rMSK
	sw	rTMP, 0(rGLB)

	/* set new PLL values */
	lw	rTMP, 0(rCTR)                     /* RTL838X_PLL_*_CTRL0_REG */
	li	rSEL, 0xfff0000f /* !(RTL838X_PLL_CMU_CTRL0_DIVN2 | RTL838X_PLL_CMU_CTRL0_NCODE_IN) */
	and	rTMP, rTMP, rSEL
	or	rTMP, rTMP, $a1                            /* argv[1]: ctrl0 */
	sw	rTMP, 0(rCTR)
	lw	rTMP, 4(rCTR)                     /* RTL838X_PLL_*_CTRL1_REG */
	li	rSEL, 0xe3ffffff /* !(RTL838X_PLL_CMU_CTRL1_DIVN3_SEL | RTL838X_PLL_CMU_CTRL1_DIVN2_SELB) */
	and	rTMP, rTMP, rSEL
	or	rTMP, rTMP, $a2                            /* argv[2]: ctrl1 */
	sw	rTMP, 4(rCTR)

	/* enable PLL (will reset it and clear ready status) */
	nor	rMSK, rMSK, 0
	lw	rTMP, 0(rGLB)
	or	rTMP, rTMP, rMSK
	sw	rTMP, 0(rGLB)

	/* wait for PLL to become ready */
wait_ready:
	lw	rTMP, 0(rGLB)
	and	rTMP, rTMP, rSLP
	bne	rTMP, $0, wait_ready
	sync                                       /* Note branch delay slot */

	/* branch to post processing */
	ori	rTMP, $0, CLK_CPU
	beq	$a0, rTMP, post_cpu
	ori	rTMP, $0, CLK_MEM                  /* Note branch delay slot */
	beq	$a0, rTMP, post_mem
	nop                                        /* Note branch delay slot */
post_lxb:
	jr	$ra
	nop                                        /* Note branch delay slot */
post_mem:
	jr	$ra
	nop                                        /* Note branch delay slot */
post_cpu:
	/* stabilize clock to avoid crash, empirically determined */
	ori	rSLP, $0, 0x3000
wait_cpu:
	bnez	rSLP, wait_cpu
	addiu	rSLP, rSLP, -1                     /* Note branch delay slot */

	/* switch CPU to PLL clock */
	ori	rMSK, $0, RTL838X_PLL_GLB_CTRL_CPU_PLL_SC_MUX
	sync
	lw	rTMP, 0(rGLB)
	or	rTMP, rTMP, rMSK
	sw	rTMP, 0(rGLB)
	sync
	jr	$ra
	nop                                        /* Note branch delay slot */

.end	rtcl_838x_dram_set_rate

/* SRAM Corruption/Correctness detection canary. Do not delete. */
	.word RTL_SRAM_SET_PLL_RATE_CANARY
.globl	rtcl_838x_dram_size
rtcl_838x_dram_size:
	.word .-rtcl_838x_dram_start
