/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Realtek RTL839X SRAM clock setters
 * Copyright (C) 2022 Markus Stockhausen <markus.stockhausen@gmx.de>
 */

#include <asm/mipsregs.h>
#include <dt-bindings/clock/rtl83xx-clk.h>

#include "clk-rtl83xx.h"

#define RTL839X_SWITCHCORE_BASE_ADDR    (0xbb000000) /* KSEG1(0x1b000000) */
#define RTL839X_SRAM_BASE               (0x9f000000) /* KSEG0(0x1f000000) */

#define rGLB	$t0
#define rCTR	$t1
#define rMSK	$t2
#define rSLP1	$t3
#define rSLP2	$t4
#define rSLP3	$t5
#define rTMP	$t6
#define rCP0	$t7

.set	noreorder

.globl	rtcl_839x_dram_start
rtcl_839x_dram_start:

/*
 * Functions start here and should avoid access to normal memory. REMARK! Do not forget about
 * stack pointer and dirty caches that might interfere.
 */

.globl	rtcl_839x_dram_set_rate
.ent	rtcl_839x_dram_set_rate
rtcl_839x_dram_set_rate:

#ifdef CONFIG_RTL839X

	/* disable MIPS 34K branch and return prediction */
        mfc0    rCP0, CP0_CONFIG, 7
        ori     rTMP, rCP0, 0xc
        mtc0    rTMP, CP0_CONFIG, 7

	li	rCTR, RTL839X_SWITCHCORE_BASE_ADDR
	addiu	rGLB, rCTR, RTL839X_PLL_GLB_CTRL_REG
	ori	rTMP, $0, CLK_CPU
	beq	$a0, rTMP, pre_cpu
	ori	rTMP, $0, CLK_MEM                  /* Note branch delay slot */
	beq	$a0, rTMP, pre_mem
	nop                                        /* Note branch delay slot */
pre_lxb:
	li	rSLP1, 0x400000
	li	rSLP2, 0x400000
	li	rSLP3, 0x400000
	addiu	rCTR, rCTR, RTL839X_PLL_LXB_CTRL0_REG
	b	main_set
	ori	rMSK, $0, RTL839X_PLL_GLB_CTRL_LXB_CLKSEL /* Note branch delay slot */
pre_mem:
	/* try to avoid memory access with simple 64K data cache flush */
	li	rMSK, RTL839X_SRAM_BASE
	li	rTMP, 2048
pre_flush:
	lw	$0, 0(rMSK)
	addiu	rMSK, rMSK, 32
	addiu	rTMP, rTMP, -1
	bne	rTMP, $0, pre_flush
	lw	$0, -4(rMSK)                       /* Note branch delay slot */

	li	rSLP1, 0x10000
	li	rSLP2, 0x10000
	li	rSLP3, 0x10000
	addiu	rCTR, rCTR, RTL839X_PLL_MEM_CTRL0_REG
	b	main_set
	ori	rMSK, $0, RTL839X_PLL_GLB_CTRL_MEM_CLKSEL /* Note branch delay slot */
pre_cpu:
	li	rSLP1, 0x1000
	li	rSLP2, 0x1000
	li	rSLP3, 0x200
	addiu	rCTR, rCTR, RTL839X_PLL_CPU_CTRL0_REG
	ori	rMSK, $0, RTL839X_PLL_GLB_CTRL_CPU_CLKSEL
main_set:
	/* switch to fixed clock */
	sync
	lw	rTMP, 0(rGLB)
	sync
	or	rTMP, rTMP, rMSK
	sync
	sw	rTMP, 0(rGLB)

	/* wait until fixed clock in use */
	or	rTMP, rSLP1, $0
wait_fixclock:
	bnez	rTMP, wait_fixclock
	addiu	rTMP, rTMP, -1                     /* Note branch delay slot */

	/* set new PLL values */
	sync
	sw	$a1, 0(rCTR)
	sw	$a2, 4(rCTR)
	sync

	/* wait for value takeover */
	or	rTMP, rSLP2, $0
wait_pll:
	bnez	rTMP, wait_pll
	addiu	rTMP, rTMP, -1                     /* Note branch delay slot */

	/* switch back to PLL clock*/
	nor	rMSK, rMSK, $0
	sync
	lw	rTMP, 0(rGLB)
	sync
	and	rTMP, rTMP, rMSK
	sync
	sw	rTMP, 0(rGLB)

	/* wait until PLL clock in use */
	or	rTMP, rSLP3, $0
wait_pllclock:
	bnez	rTMP, wait_pllclock
	addiu	rTMP, rTMP, -1                     /* Note branch delay slot */

	/* restore branch prediction */
	mtc0    rCP0, CP0_CONFIG, 7
	jr	$ra
	nop                                        /* Note branch delay slot */

#else /* !CONFIG_RTL839X */

	jr	$ra
	nop                                        /* Note branch delay slot */

#endif

.end	rtcl_839x_dram_set_rate

/* SRAM Corruption/Correctness detection canary. Do not delete. */
	 .word RTL_SRAM_SET_PLL_RATE_CANARY
.globl	rtcl_839x_dram_size
rtcl_839x_dram_size:
	.word .-rtcl_839x_dram_start
