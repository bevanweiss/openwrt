// SPDX-License-Identifier: GPL-2.0-only
/*
 * based on the original BSP by
 * Copyright (C) 2006-2012 Tony Wu (tonywu@realtek.com)
 */

#include <asm/addrspace.h>
#include <asm/cpu-features.h>
#include <asm/io.h>
#include <asm/mach-realtek/otto.h>
#include <asm/machine.h>
#include <asm/mips-cps.h>
#include <asm/mips-gic.h>
#include <asm/mipsregs.h>
#include <asm/prom.h>
#include <asm/setup.h>
#include <asm/smp-ops.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/init.h>
#include <linux/mod_devicetable.h>
#include <linux/printk.h>
#include <linux/regmap.h>
#include <linux/string.h>
#include <linux/types.h>

/* The chip identification bits reside in the multiple parts of the SoC. */
/* RTL838x */
#define RTL838X_SWITCHCORE_BASE_ADDR    (0x1b000000)

#define RTL838X_INT_RW_CTRL_REG         (0x0058)
/* Reserved                                     31 - 2 */
#define RTL838X_INT_RW_CTRL_WRITE_EN            BIT(1)
#define RTL838X_INT_RW_CTRL_READ_EN             BIT(0)

#define RTL838X_MODEL_NAME_INFO_REG     (0x00d4)
#define RTL838X_MODEL_NAME_INFO_ID              GENMASK(31, 16)
#define RTL838X_MODEL_NAME_INFO_MODEL0          GENMASK(15, 11)
#define RTL838X_MODEL_NAME_INFO_MODEL1          GENMASK(10, 6)
#define RTL838X_MODEL_NAME_INFO_MODEL2          GENMASK(5, 1)
/* Reserved                                     0 */

#define RTL838X_CHIP_INFO_REG           (0x00d8)
#define RTL838X_CHIP_INFO_EN                    GENMASK(31, 28)
#define RTL838X_CHIP_INFO_EN_KEY                        0xa
/* Reserved                                     27 - 21 */
#define RTL838X_CHIP_INFO_VER                   GENMASK(20, 16)
#define RTL838X_CHIP_INFO_ID                    GENMASK(15, 0)

#define RTL838X_MODEL_INFO_REG          (0x00dc)
/* Reserved                                     31 - 6 */
#define RTL838X_MODEL_INFO_ID                   GENMASK(5, 0)

/* RTL839x */
#define RTL839X_SWITCHCORE_BASE_ADDR    (0x1b000000)

#define RTL839X_MODEL_NAME_INFO_REG     (0x0ff0)
#define RTL839X_MODEL_NAME_INFO_ID              GENMASK(31, 16)
#define RTL839X_MODEL_NAME_INFO_MODEL0          GENMASK(15, 11)
#define RTL839X_MODEL_NAME_INFO_MODEL1          GENMASK(10, 6)
/* Reserved                                     5 - 0 */

#define RTL839X_CHIP_INFO_REG           (0x0ff4)
#define RTL839X_CHIP_INFO_EN                    GENMASK(31, 28)
#define RTL839X_CHIP_INFO_EN_KEY                        0xa
/* Reserved                                     27 - 21 */
#define RTL839X_CHIP_INFO_VER                   GENMASK(20, 16)
#define RTL839X_CHIP_INFO_ID                    GENMASK(15, 0)

/* RTL930x */
#define RTL930X_SWITCHCORE_BASE_ADDR    (0x1b000000)

#define RTL930X_MODEL_NAME_INFO_REG     (0x0004)
#define RTL930X_MODEL_NAME_INFO_ID              GENMASK(31, 16)
#define RTL930X_MODEL_NAME_INFO_MODEL0          GENMASK(15, 11)
#define RTL930X_MODEL_NAME_INFO_MODEL1          GENMASK(10, 6)
#define RTL930X_MODEL_NAME_INFO_MODEL2          GENMASK(5, 4)
#define RTL930X_MODEL_NAME_INFO_VID             GENMASK(3, 0)

#define RTL930X_CHIP_INFO_REG           (0x0008)
#define RTL930X_CHIP_INFO_VID                   GENMASK(31, 28)
#define RTL930X_CHIP_INFO_MCID                  GENMASK(27, 24)
/* Reserved                                     23 - 20 */
#define RTL930X_CHIP_INFO_EN                    GENMASK(19, 16)
#define RTL930X_CHIP_INFO_EN_KEY                        0xa
#define RTL930X_CHIP_INFO_ID                    GENMASK(15, 0)

/* RTL931x */
#define RTL931X_SWITCHCORE_BASE_ADDR    (0x1b000000)

#define RTL931X_MODEL_NAME_INFO_REG     (0x0004)
#define RTL931X_MODEL_NAME_INFO_ID              GENMASK(31, 16)
#define RTL931X_MODEL_NAME_INFO_MODEL0          GENMASK(15, 11)
#define RTL931X_MODEL_NAME_INFO_MODEL1          GENMASK(10, 6)
#define RTL931X_MODEL_NAME_INFO_MODEL2          GENMASK(5, 4)
#define RTL931X_MODEL_NAME_INFO_VID             GENMASK(3, 0)

#define RTL931X_CHIP_INFO_REG           (0x0008)
#define RTL931X_CHIP_INFO_VID                   GENMASK(31, 28)
#define RTL931X_CHIP_INFO_MCID                  GENMASK(27, 24)
#define RTL931X_CHIP_INFO_BOID                  GENMASK(23, 20)
#define RTL931X_CHIP_INFO_EN                    GENMASK(19, 16)
#define RTL931X_CHIP_INFO_EN_KEY                        0xa
#define RTL931X_CHIP_INFO_ID                    GENMASK(15, 0)

/* Realtek family identifiers */
#define RTL_OTTO_FAMILY_MASK                    GENMASK(15, 4)
#define RTL_OTTO_FAMILY_UNKNOWN                         0x0000
#define RTL_OTTO_FAMILY_RTL838X                         0x8380
#define RTL_OTTO_FAMILY_RTL839X                         0x8390
#define RTL_OTTO_FAMILY_RTL930X                         0x9300
#define RTL_OTTO_FAMILY_RTL931X                         0x9310

#define REALTEK_NAME_SUFFIX_LEN         (5)
#define REALTEK_SOC_CHIP_INFO_LEN       (96)
#define REALTEK_SOC_INFO_NAME_LEN       (27 + REALTEK_SOC_CHIP_INFO_LEN + REALTEK_NAME_SUFFIX_LEN)
#define RELATEK_SOC_INFO_NAME_FORMAT    "RTL%04x%s v%d (%s)"
#define DELETE_CHAR  (0x7f) /* DELETE character to have an empty char */
#define REALTEK_MODEL_NAME_CHAR_XLATE(x) \
        ((((x) > ('A' - 'A')) && ((x) <= ('Z' - 'A'))) ? 'A' + (x) - 1 : DELETE_CHAR)

struct realtek_soc_info {
	u32 id;
	u16 model;
	u16 family;
	char name[REALTEK_SOC_INFO_NAME_LEN];
};

struct realtek_soc_data {
	u16 family;
	u32 family_mask;
	void (*unlock)(void);
	void (*identify)(const struct realtek_soc_data *, struct realtek_soc_info *);
};

static struct realtek_soc_info rtl_soc_info;

/* XXX For legacy reasons only, remove when ready */
struct rtl83xx_soc_info soc_info;

static u32 __init rtl_otto_read(u32 reg)
{
	return ioread32((const void __iomem *)CKSEG1ADDR(reg));
}

static void __init rtl_otto_write(u32 val, u32 reg)
{
	iowrite32(val, (void __iomem *)CKSEG1ADDR(reg));
}

static void __init _remove_delete_char(char *buf)
{
	for (int i = 0, j = 0; buf[j] != '\0'; j++) {
		if (buf[j] == DELETE_CHAR)
			continue;

		buf[i] = buf[j];
		i++;
	}
}

static void __init print_model_suffix(u32 model0, u32 model1, u32 model2, char *buf)
{
	buf[3] = REALTEK_MODEL_NAME_CHAR_XLATE(model2);
	buf[2] = REALTEK_MODEL_NAME_CHAR_XLATE(model1);
	buf[1] = ((buf[3] != DELETE_CHAR) || (buf[2] != DELETE_CHAR)) ? '-' : DELETE_CHAR;
	buf[0] = REALTEK_MODEL_NAME_CHAR_XLATE(model0);

	buf[REALTEK_NAME_SUFFIX_LEN - 1] = '\0';

	_remove_delete_char(buf);
}

#ifdef CONFIG_MIPS_MT_SMP
/*
 * This is needed by the VPE loader code, just set it to 0 and assume
 * that the firmware hardcodes this value to something useful.
 */
unsigned long physical_memsize = 0L;

extern const struct plat_smp_ops vsmp_smp_ops;

static void __init rtl_init_secondary(void)
{
#ifndef CONFIG_CEVT_R4K
/*
 * These devices are low on resources. There might be the chance that CEVT_R4K
 * is not enabled in kernel build. Nevertheless the timer and interrupt 7 might
 * be active by default after startup of secondary VPE. With no registered
 * handler that leads to continuous unhandeled interrupts. In this case disable
 * counting (DC) in the core and confirm a pending interrupt.
 */
	write_c0_cause(read_c0_cause() | CAUSEF_DC);
	write_c0_compare(0);
#endif /* CONFIG_CEVT_R4K */
/*
 * Enable all CPU interrupts, as everything is managed by the external
 * controller. TODO: Standard vsmp_init_secondary() has special treatment for
 * Malta if external GIC is available. Maybe we need this too.
 */
	if (mips_gic_present())
		pr_warn("%s: GIC present. Maybe interrupt enabling required.\n", __func__);
	else
		set_c0_status(ST0_IM);
}
#endif /* CONFIG_MIPS_MT_SMP */

static const void __init *realtek_fixup_fdt(const void *fdt, const void *data)
{
	const struct realtek_soc_data *rtl_soc_data = data;

	if (!rtl_soc_data) {
		pr_warn("Unknown SoC data!\n");
		return fdt;
	}

	if (rtl_soc_data->unlock)
		rtl_soc_data->unlock();

	if (rtl_soc_data->identify)
	      rtl_soc_data->identify(rtl_soc_data, &rtl_soc_info);

	if (rtl_soc_info.family != rtl_soc_data->family) {
		pr_warn("WARNING! Detected SoC ID '0x%08x' is not part of family: '%04x'\n",
		        rtl_soc_info.id, rtl_soc_data->family);

		rtl_soc_info.family = RTL_OTTO_FAMILY_UNKNOWN;
		snprintf(rtl_soc_info.name, REALTEK_SOC_INFO_NAME_LEN, "Unknown");
	}

	system_type = rtl_soc_info.name;
	pr_info("SoC: Realtek %s\n", system_type);

	/* TODO For legacy reasons, remove when ready */
	soc_info.name = rtl_soc_info.name;
	soc_info.id = rtl_soc_info.model;
	soc_info.family = rtl_soc_info.family;

	return fdt;
}

static void __init realtek_register_smp_ops(void)
{
	if (!register_cps_smp_ops())
		return;

#ifdef CONFIG_MIPS_MT_SMP
	if (cpu_has_mipsmt) {
		extern const struct plat_smp_ops vsmp_smp_ops;
		struct plat_smp_ops rtl_smp_ops;

		rtl_smp_ops = vsmp_smp_ops;
		rtl_smp_ops.init_secondary = rtl_init_secondary;
		register_smp_ops(&rtl_smp_ops);

		return;
	}
#endif

	register_up_smp_ops();
}

static void __init rtl838x_identify(const struct realtek_soc_data *rtl_soc_data,
                                    struct realtek_soc_info *rtl_soc_info)
{
	char extra_info[REALTEK_SOC_CHIP_INFO_LEN];
	char buf[REALTEK_NAME_SUFFIX_LEN];
	u32 model_info;
	u32 chip_info;

	chip_info = rtl_otto_read(RTL838X_SWITCHCORE_BASE_ADDR + RTL838X_CHIP_INFO_REG);
	model_info = rtl_otto_read(RTL838X_SWITCHCORE_BASE_ADDR + RTL838X_MODEL_INFO_REG);
	rtl_soc_info->id = rtl_otto_read(RTL838X_SWITCHCORE_BASE_ADDR + RTL838X_MODEL_NAME_INFO_REG);
	rtl_soc_info->model = FIELD_GET(RTL838X_MODEL_NAME_INFO_ID, rtl_soc_info->id);
	rtl_soc_info->family = rtl_soc_info->model & rtl_soc_data->family_mask;

	print_model_suffix(FIELD_GET(RTL838X_MODEL_NAME_INFO_MODEL0, rtl_soc_info->id),
	                   FIELD_GET(RTL838X_MODEL_NAME_INFO_MODEL1, rtl_soc_info->id),
	                   FIELD_GET(RTL838X_MODEL_NAME_INFO_MODEL2, rtl_soc_info->id),
	                   buf),

	snprintf(extra_info, REALTEK_SOC_CHIP_INFO_LEN,
	         "[0x%08x] ID: 0x%04x model: 0x%04x",
		 rtl_soc_info->id,
	         (u16)FIELD_GET(RTL838X_CHIP_INFO_ID, chip_info),
	         (u16)FIELD_GET(RTL838X_MODEL_INFO_ID, model_info));

	snprintf(rtl_soc_info->name, REALTEK_SOC_INFO_NAME_LEN,
	         RELATEK_SOC_INFO_NAME_FORMAT,
	         rtl_soc_info->model,
	         buf,
	         (u8)FIELD_GET(RTL838X_CHIP_INFO_VER, chip_info),
	         extra_info);
}

static void __init rtl839x_identify(const struct realtek_soc_data *rtl_soc_data,
                                    struct realtek_soc_info *rtl_soc_info)
{
	char extra_info[REALTEK_SOC_CHIP_INFO_LEN];
	char buf[REALTEK_NAME_SUFFIX_LEN];
	u32 chip_info;

	chip_info = rtl_otto_read(RTL839X_SWITCHCORE_BASE_ADDR + RTL839X_CHIP_INFO_REG);
	rtl_soc_info->id = rtl_otto_read(RTL839X_SWITCHCORE_BASE_ADDR + RTL839X_MODEL_NAME_INFO_REG);
	rtl_soc_info->model = FIELD_GET(RTL839X_MODEL_NAME_INFO_ID, rtl_soc_info->id);
	rtl_soc_info->family = rtl_soc_info->model & rtl_soc_data->family_mask;

	snprintf(extra_info, REALTEK_SOC_CHIP_INFO_LEN,
		 "[0x%08x] ID: 0x%04x",
		 rtl_soc_info->id,
	         (u16)FIELD_GET(RTL839X_CHIP_INFO_ID, chip_info));

	print_model_suffix(FIELD_GET(RTL838X_MODEL_NAME_INFO_MODEL0, rtl_soc_info->id),
	                   FIELD_GET(RTL838X_MODEL_NAME_INFO_MODEL1, rtl_soc_info->id),
	                   0,
	                   buf),

	snprintf(rtl_soc_info->name, REALTEK_SOC_INFO_NAME_LEN,
	         RELATEK_SOC_INFO_NAME_FORMAT,
	         rtl_soc_info->model,
	         buf,
	         (u8)FIELD_GET(RTL839X_CHIP_INFO_VER, chip_info),
	         extra_info);
}

static void __init rtl930x_identify(const struct realtek_soc_data *rtl_soc_data,
                                    struct realtek_soc_info *rtl_soc_info)
{
	char extra_info[REALTEK_SOC_CHIP_INFO_LEN];
	char buf[REALTEK_NAME_SUFFIX_LEN];
	u32 chip_info;

	chip_info = rtl_otto_read(RTL930X_SWITCHCORE_BASE_ADDR + RTL930X_CHIP_INFO_REG);
	rtl_soc_info->id = rtl_otto_read(RTL930X_SWITCHCORE_BASE_ADDR + RTL930X_MODEL_NAME_INFO_REG);
	rtl_soc_info->model = FIELD_GET(RTL930X_MODEL_NAME_INFO_ID, rtl_soc_info->id);
	rtl_soc_info->family = rtl_soc_info->model & rtl_soc_data->family_mask;

	print_model_suffix(FIELD_GET(RTL930X_MODEL_NAME_INFO_MODEL0, rtl_soc_info->id),
	                   FIELD_GET(RTL930X_MODEL_NAME_INFO_MODEL1, rtl_soc_info->id),
	                   FIELD_GET(RTL930X_MODEL_NAME_INFO_MODEL2, rtl_soc_info->id),
	                   buf),

	snprintf(extra_info, REALTEK_SOC_CHIP_INFO_LEN,
		 "[0x%08x] variant: %s VER: %d MCID: 0x%x ID: 0x%04x",
		 rtl_soc_info->id,
		 FIELD_GET(RTL930X_MODEL_NAME_INFO_MODEL2, rtl_soc_info->id) == 0x1 ? "2G5" : "10G",
	         (u8)FIELD_GET(RTL930X_CHIP_INFO_VID, chip_info),
	         (u8)FIELD_GET(RTL930X_CHIP_INFO_MCID, chip_info),
	         (u16)FIELD_GET(RTL930X_CHIP_INFO_ID, chip_info));

	snprintf(rtl_soc_info->name, REALTEK_SOC_INFO_NAME_LEN,
	         RELATEK_SOC_INFO_NAME_FORMAT,
	         rtl_soc_info->model,
	         buf,
	         (u8)FIELD_GET(RTL930X_MODEL_NAME_INFO_VID, rtl_soc_info->id),
	         extra_info);
}

static void __init rtl931x_identify(const struct realtek_soc_data *rtl_soc_data,
                                    struct realtek_soc_info *rtl_soc_info)
{
	char extra_info[REALTEK_SOC_CHIP_INFO_LEN];
	char buf[REALTEK_NAME_SUFFIX_LEN];
	u32 chip_info;

	chip_info = rtl_otto_read(RTL931X_SWITCHCORE_BASE_ADDR + RTL931X_CHIP_INFO_REG);
	rtl_soc_info->id = rtl_otto_read(RTL931X_SWITCHCORE_BASE_ADDR + RTL931X_MODEL_NAME_INFO_REG);
	rtl_soc_info->model = FIELD_GET(RTL931X_MODEL_NAME_INFO_ID, rtl_soc_info->id);
	rtl_soc_info->family = rtl_soc_info->model & rtl_soc_data->family_mask;

	print_model_suffix(FIELD_GET(RTL931X_MODEL_NAME_INFO_MODEL0, rtl_soc_info->id),
	                   FIELD_GET(RTL931X_MODEL_NAME_INFO_MODEL1, rtl_soc_info->id),
	                   FIELD_GET(RTL931X_MODEL_NAME_INFO_MODEL2, rtl_soc_info->id),
	                   buf),

	snprintf(extra_info, REALTEK_SOC_CHIP_INFO_LEN,
	         "[0x%08x] VER: %d MCID: 0x%x BOND: 0x%x ID: 0x%04x",
		 rtl_soc_info->id,
	         (u8)FIELD_GET(RTL931X_CHIP_INFO_VID, chip_info),
	         (u8)FIELD_GET(RTL931X_CHIP_INFO_MCID, chip_info),
	         (u8)FIELD_GET(RTL931X_CHIP_INFO_BOID, chip_info),
	         (u16)FIELD_GET(RTL931X_CHIP_INFO_ID, chip_info));

	snprintf(rtl_soc_info->name, REALTEK_SOC_INFO_NAME_LEN,
	         RELATEK_SOC_INFO_NAME_FORMAT,
	         rtl_soc_info->model,
	         buf,
	         (u8)FIELD_GET(RTL931X_MODEL_NAME_INFO_VID, rtl_soc_info->id),
	         extra_info);
}

static void __init rtl838x_unlock(void)
{
	rtl_otto_write(FIELD_PREP(RTL838X_CHIP_INFO_EN, RTL838X_CHIP_INFO_EN_KEY),
	               RTL838X_SWITCHCORE_BASE_ADDR + RTL838X_CHIP_INFO_REG);

	rtl_otto_write(RTL838X_INT_RW_CTRL_WRITE_EN | RTL838X_INT_RW_CTRL_READ_EN,
	               RTL838X_SWITCHCORE_BASE_ADDR + RTL838X_INT_RW_CTRL_REG);
}

static void __init rtl839x_unlock(void)
{
	rtl_otto_write(FIELD_PREP(RTL839X_CHIP_INFO_EN, RTL839X_CHIP_INFO_EN_KEY),
	               RTL839X_SWITCHCORE_BASE_ADDR + RTL839X_CHIP_INFO_REG);
}

static void __init rtl930x_unlock(void)
{
	rtl_otto_write(FIELD_PREP(RTL930X_CHIP_INFO_EN, RTL930X_CHIP_INFO_EN_KEY),
	               RTL930X_SWITCHCORE_BASE_ADDR + RTL930X_CHIP_INFO_REG);
}

static void __init rtl931x_unlock(void)
{
	rtl_otto_write(FIELD_PREP(RTL931X_CHIP_INFO_EN, RTL931X_CHIP_INFO_EN_KEY),
	               RTL931X_SWITCHCORE_BASE_ADDR + RTL931X_CHIP_INFO_REG);
}

static const struct realtek_soc_data rtl838x_soc __initconst = {
	.family = RTL_OTTO_FAMILY_RTL838X,
	.family_mask = RTL_OTTO_FAMILY_MASK,
	.unlock = rtl838x_unlock,
	.identify = rtl838x_identify,
};

static const struct realtek_soc_data rtl839x_soc __initconst = {
	.family = RTL_OTTO_FAMILY_RTL839X,
	.family_mask = RTL_OTTO_FAMILY_MASK,
	.unlock = rtl839x_unlock,
	.identify = rtl839x_identify,
};

static const struct realtek_soc_data rtl930x_soc __initconst = {
	.family = RTL_OTTO_FAMILY_RTL930X,
	.family_mask = RTL_OTTO_FAMILY_MASK,
	.unlock = rtl930x_unlock,
	.identify = rtl930x_identify,
};

static const struct realtek_soc_data rtl931x_soc __initconst = {
	.family = RTL_OTTO_FAMILY_RTL931X,
	.family_mask = RTL_OTTO_FAMILY_MASK,
	.unlock = rtl931x_unlock,
	.identify = rtl931x_identify,
};

static const struct of_device_id realtek_board_ids[] __initconst = {
	{ .compatible = "realtek,otto-soc" },
	/* RTL838x */
	{ .compatible = "realtek,maple-soc", &rtl838x_soc },
	{ .compatible = "realtek,rtl838x-soc", &rtl838x_soc },
	{ .compatible = "realtek,rtl8380-soc", &rtl838x_soc },
	{ .compatible = "realtek,rtl8381-soc", &rtl838x_soc },
	{ .compatible = "realtek,rtl8382-soc", &rtl838x_soc },
	/* RTL839x */
	{ .compatible = "realtek,cypress-soc", &rtl839x_soc },
	{ .compatible = "realtek,rtl839x-soc", &rtl839x_soc },
	{ .compatible = "realtek,rtl8390-soc", &rtl839x_soc },
	{ .compatible = "realtek,rtl8391-soc", &rtl839x_soc },
	{ .compatible = "realtek,rtl8392-soc", &rtl839x_soc },
	{ .compatible = "realtek,rtl8393-soc", &rtl839x_soc },
	/* RTL930x */
	{ .compatible = "realtek,longan-soc", &rtl930x_soc },
	{ .compatible = "realtek,rtl930x-soc", &rtl930x_soc },
	{ .compatible = "realtek,rtl9300-soc", &rtl930x_soc },
	{ .compatible = "realtek,rtl9301-soc", &rtl930x_soc },
	{ .compatible = "realtek,rtl9302-soc", &rtl930x_soc },
	{ .compatible = "realtek,rtl9302a-soc", &rtl930x_soc },
	{ .compatible = "realtek,rtl9302b-soc", &rtl930x_soc },
	{ .compatible = "realtek,rtl9302c-soc", &rtl930x_soc },
	{ .compatible = "realtek,rtl9302d-soc", &rtl930x_soc },
	{ .compatible = "realtek,rtl9302e-soc", &rtl930x_soc  },
	{ .compatible = "realtek,rtl9302f-soc", &rtl930x_soc  },
	{ .compatible = "realtek,rtl9303-soc", &rtl930x_soc  },
	/* RTL931x */
	{ .compatible = "realtek,mango-soc", &rtl931x_soc },
	{ .compatible = "realtek,rtl931x-soc", &rtl931x_soc },
	{ .compatible = "realtek,rtl9310-soc", &rtl931x_soc },
	{ .compatible = "realtek,rtl9311-soc", &rtl931x_soc },
	{ .compatible = "realtek,rtl9313-soc", &rtl931x_soc },
	{ /* sentinel */ }
};

MIPS_MACHINE(realtek) = {
	.matches          = realtek_board_ids,
	.fixup_fdt        = realtek_fixup_fdt,
	.register_smp_ops = realtek_register_smp_ops,
};
