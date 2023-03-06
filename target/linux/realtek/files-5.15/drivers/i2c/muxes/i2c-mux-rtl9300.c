// SPDX-License-Identifier: GPL-2.0-only
/*
 * I2C multiplexer for the 2 I2C Masters of the RTL9300
 * with up to 8 channels each, but which are not entirely
 * independent of each other
 */
#include <linux/i2c-mux.h>
#include <linux/module.h>
#include <linux/mux/consumer.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>

#include "../busses/i2c-rtl9300.h"

#define REG(mst, x)	(mux->base + x + (mst ? mux->i2c->mst2_offset : 0))
#define REG_MASK(mst, clear, set, reg)	\
			writel((readl(REG((mst),(reg))) & ~(clear)) | (set), REG((mst),(reg)))

struct rtl9300_mux {
	void __iomem *base;
	struct device *dev;
	struct i2c_adapter *parent;
	struct rtl9300_i2c * i2c;
};

struct i2c_mux_data  {
	int sda_first_pin;
	int sda_last_pin;
	int (*i2c_mux_select)(struct i2c_mux_core *muxc, u32 chan);
	int (*i2c_mux_deselect)(struct i2c_mux_core *muxc, u32 chan);
	void (*sda_sel)(struct i2c_mux_core *muxc, int pin);
};

static int rtl9300_i2c_mux_select(struct i2c_mux_core *muxc, u32 chan)
{
	struct rtl9300_mux *mux = i2c_mux_priv(muxc);

	/* Set SDA pin */
	REG_MASK(mux->i2c->scl_num, RTL9300_I2C_CTRL1_SDA_OUT_SEL_MASK,
		 chan << RTL9300_I2C_CTRL1_SDA_OUT_SEL, RTL9300_I2C_CTRL1);

	return 0;
}

static int rtl9310_i2c_mux_select(struct i2c_mux_core *muxc, u32 chan)
{
	struct rtl9300_mux *mux = i2c_mux_priv(muxc);

	/* Set SDA pin */
	REG_MASK(mux->i2c->scl_num, RTL9310_I2C_CTRL_SDA_OUT_SEL_MASK,
		 chan << RTL9310_I2C_CTRL_SDA_OUT_SEL, RTL9310_I2C_CTRL);

	return 0;
}

static int rtl9300_i2c_mux_deselect(struct i2c_mux_core *muxc, u32 chan)
{
	return 0;
}

static void rtl9300_sda_sel(struct i2c_mux_core *muxc, int pin)
{
	struct rtl9300_mux *mux = i2c_mux_priv(muxc);
	u32 v;

	/* Set SDA pin to I2C functionality */
	v = readl(REG(0, RTL9300_I2C_MST_GLB_CTRL));
	v |= BIT(pin);
	writel(v, REG(0, RTL9300_I2C_MST_GLB_CTRL));
}

static void rtl9310_sda_sel(struct i2c_mux_core *muxc, int pin)
{
	struct rtl9300_mux *mux = i2c_mux_priv(muxc);
	u32 v;

	/* Set SDA pin to I2C functionality */
	v = readl(REG(0, RTL9310_I2C_MST_IF_SEL));
	v |= BIT(pin);
	writel(v, REG(0, RTL9310_I2C_MST_IF_SEL));
}

static struct device_node *mux_parent_adapter(struct device *dev, struct rtl9300_mux *mux)
{
	struct device_node *node = dev->of_node;
	struct device_node *parent_np;
	struct i2c_adapter *parent;

	parent_np = of_parse_phandle(node, "i2c-parent", 0);
	if (!parent_np) {
		dev_err(dev, "Cannot parse i2c-parent\n");
		return ERR_PTR(-ENODEV);
	}
	parent = of_find_i2c_adapter_by_node(parent_np);
	of_node_put(parent_np);
	if (!parent)
		return ERR_PTR(-EPROBE_DEFER);

	if (!(of_device_is_compatible(parent_np, "realtek,rtl930x-i2c") ||
	    of_device_is_compatible(parent_np, "realtek,rtl931x-i2c"))){
		dev_err(dev, "I2C parent not an RTL93xx I2C controller\n");
		return ERR_PTR(-ENODEV);
	}

	mux->parent = parent;
	mux->i2c = (struct rtl9300_i2c *)i2c_get_adapdata(parent);
	mux->base = mux->i2c->base;

	return parent_np;
}

struct i2c_mux_data rtl930x_i2c_mux_data = {
	.sda_first_pin = 9,
	.sda_last_pin = 16,
	.i2c_mux_select = rtl9300_i2c_mux_select,
	.i2c_mux_deselect = rtl9300_i2c_mux_deselect,
	.sda_sel = rtl9300_sda_sel,
};

struct i2c_mux_data rtl931x_i2c_mux_data = {
	.sda_first_pin = 0,
	.sda_last_pin = 16,
	.i2c_mux_select = rtl9310_i2c_mux_select,
	.i2c_mux_deselect = rtl9300_i2c_mux_deselect,
	.sda_sel = rtl9310_sda_sel,
};

static const struct of_device_id rtl93xx_i2c_mux_of_match[] = {
	{ .compatible = "realtek,i2c-mux-longan", .data = &rtl930x_i2c_mux_data},
	{ .compatible = "realtek,i2c-mux-rtl930x", .data = &rtl930x_i2c_mux_data},
	{ .compatible = "realtek,i2c-mux-rtl9301", .data = &rtl930x_i2c_mux_data},
	{ .compatible = "realtek,i2c-mux-rtl9302", .data = &rtl930x_i2c_mux_data},
	{ .compatible = "realtek,i2c-mux-rtl9302a", .data = &rtl930x_i2c_mux_data},
	{ .compatible = "realtek,i2c-mux-rtl9302b", .data = &rtl930x_i2c_mux_data},
	{ .compatible = "realtek,i2c-mux-rtl9302c", .data = &rtl930x_i2c_mux_data},
	{ .compatible = "realtek,i2c-mux-rtl9302e", .data = &rtl930x_i2c_mux_data},
	{ .compatible = "realtek,i2c-mux-rtl9302d", .data = &rtl930x_i2c_mux_data},
	{ .compatible = "realtek,i2c-mux-rtl9302f", .data = &rtl930x_i2c_mux_data},
	{ .compatible = "realtek,i2c-mux-rtl9303", .data = &rtl930x_i2c_mux_data},
	{ .compatible = "realtek,i2c-mux-mango", .data = &rtl931x_i2c_mux_data},
	{ .compatible = "realtek,i2c-mux-rtl931x", .data = &rtl931x_i2c_mux_data},
	{ .compatible = "realtek,i2c-mux-rtl9311", .data = &rtl931x_i2c_mux_data},
	{ .compatible = "realtek,i2c-mux-rtl9313", .data = &rtl931x_i2c_mux_data},
	{},
};
MODULE_DEVICE_TABLE(of, rtl93xx_i2c_mux_of_match);

static int rtl9300_i2c_mux_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct device_node *parent_np;
	struct device_node *child;
	struct i2c_mux_core *muxc;
	struct rtl9300_mux *mux;
	struct i2c_mux_data *mux_data;
	int children;
	int ret;

	pr_info("%s probing I2C adapter\n", __func__);

	if (!node) {
		dev_err(dev, "No DT found\n");
		return -EINVAL;
	}

	mux = devm_kzalloc(dev, sizeof(*mux), GFP_KERNEL);
	if (!mux)
		return -ENOMEM;

	mux->dev = dev;

	mux_data = (struct i2c_mux_data *) device_get_match_data(dev);

	parent_np = mux_parent_adapter(dev, mux);
	if (IS_ERR(parent_np))
		return dev_err_probe(dev, PTR_ERR(parent_np), "i2c-parent adapter not found\n");

	pr_info("%s base memory %08x\n", __func__, (u32)mux->base);

	children = of_get_child_count(node);

	muxc = i2c_mux_alloc(mux->parent, dev, children, 0, 0,
			     mux_data->i2c_mux_select, mux_data->i2c_mux_deselect);
	if (!muxc) {
		ret = -ENOMEM;
		goto err_parent;
	}
	muxc->priv = mux;

	platform_set_drvdata(pdev, muxc);

	for_each_available_child_of_node(node, child) {
		u32 pin;

		if (of_property_read_u32(child, "sda-pin", &pin)) {
			dev_warn(dev, "SDA pin not found in DT\n");
			ret = -EINVAL;
			goto err_children;
		}
		if ((pin < mux_data->sda_first_pin) || (pin > mux_data->sda_last_pin)) {
			dev_warn(dev, "SDA pin %d not supported\n", pin);
			ret = -EINVAL;
			goto err_children;
		}
		mux_data->sda_sel(muxc, pin);

		ret = i2c_mux_add_adapter(muxc, 0, pin, 0);
		if (ret)
			goto err_children;

		dev_info(dev, "mux sda_num %d configured\n", pin);
	}

	dev_info(dev, "%d-port mux on %s adapter\n", children, mux->parent->name);

	return 0;

err_children:
	i2c_mux_del_adapters(muxc);
err_parent:
	i2c_put_adapter(mux->parent);

	return ret;
}

static int rtl9300_i2c_mux_remove(struct platform_device *pdev)
{
	struct i2c_mux_core *muxc = platform_get_drvdata(pdev);

	i2c_mux_del_adapters(muxc);
	i2c_put_adapter(muxc->parent);

	return 0;
}

static struct platform_driver rtl93xx_i2c_mux_driver = {
	.probe	= rtl9300_i2c_mux_probe,
	.remove	= rtl9300_i2c_mux_remove,
	.driver	= {
		.name	= "i2c-mux-rtl93xx",
		.of_match_table = rtl93xx_i2c_mux_of_match,
	},
};
module_platform_driver(rtl93xx_i2c_mux_driver);

MODULE_DESCRIPTION("RTL9300 I2C multiplexer driver");
MODULE_AUTHOR("Birger Koblitz");
MODULE_LICENSE("GPL v2");
