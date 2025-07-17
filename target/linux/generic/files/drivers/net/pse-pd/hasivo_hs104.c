// SPDX-License-Identifier: GPL-2.0-only
/*
 * Hasivo HS104 PoE PSE controller (I2C)
 *
 * Copyright (c) 2025 Bevan Weiss <bevan.weiss@gmail.com>
 */

#include <linux/bitfield.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/pse-pd/pse.h>
#include <linux/regmap.h>
#include <linux/leds.h>

#define HS104_MAX_CHANS		4

/* Registers */
#define HS104_REG_PW_STATUS		0x01
#define HS104_REG_INPUT_V		0x02 /* 16-bit, 0.01V */
#define HS104_REG_PORT0_A		0x04 /* 16-bit, 1mA */
#define HS104_REG_DEVID			0x0C
#define HS104_REG_PORT0_CLASS	0x0D
#define HS104_REG_PW_EN			0x14
#define HS104_REG_PROTOCOL		0x19
#define HS104_REG_TOTAL_POWER	0x1D /* 16-bit, 0.01W */
#define HS104_REG_PORT0_POWER	0x21 /* 16-bit, 0.01W */

#define HS104_DEVICE_ID		0x91

/* Command execute */
#define HS104_EXECUTE		0x40

/* Protocol encoding */
#define HS104_PROTO_MASK	0x3
#define HS104_PROTO_BT		0
#define HS104_PROTO_HIPO	1
#define HS104_PROTO_AT		2
#define HS104_PROTO_AF		3

/* Power limits (mW) */
#define HS104_AF_MW			15400
#define HS104_AT_MW			30000
#define HS104_BT_MW			60000
#define HS104_HIPO_MW		90000

#define HS104_uV_STEP		10000
#define HS104_uA_STEP		1000
#define HS104_mW_STEP		10

struct hs104_port_led {
	struct led_trigger delivering_trig;
	struct led_trigger enabled_trig;
	bool delivering;
	bool enabled;
};

struct hs104_priv {
	struct regmap *regmap;
	struct hs104_port_led port_led[HS104_MAX_CHANS];
	unsigned int port_enable_delay_ms;
};

static struct hs104_priv *
to_hs104(struct pse_controller_dev *pcdev)
{
	return (struct hs104_priv *)dev_get_drvdata(pcdev->dev);
}

static int hs104_read_be16(struct hs104_priv *priv,
			   unsigned int reg, unsigned int step)
{
	__be16 val;
	int ret;

	ret = regmap_bulk_read(priv->regmap, reg, &val, sizeof(val));
	if (ret)
		return ret;

	return (be16_to_cpu(val) & 0x3fff) * step;
}

static void
hs104_update_delivering_led(struct hs104_priv *priv, int port, bool delivering)
{
	struct hs104_port_led *pled = &priv->port_led[port];

	if (pled->delivering == delivering)
		return;

	pled->delivering = delivering;

	if (delivering)
		led_trigger_event(&pled->delivering_trig, LED_FULL);
	else
		led_trigger_event(&pled->delivering_trig, LED_OFF);
}

static void
hs104_update_enabled_led(struct hs104_priv *priv, int port, bool enabled)
{
	struct hs104_port_led *pled = &priv->port_led[port];

	if (pled->enabled == enabled)
		return;

	pled->enabled = enabled;

	if (enabled)
		led_trigger_event(&pled->enabled_trig, LED_FULL);
	else
		led_trigger_event(&pled->enabled_trig, LED_OFF);
}


/* --- PSE ops --- */

static int hs104_pi_enable(struct pse_controller_dev *pcdev, int port)
{
	struct hs104_priv *priv = to_hs104(pcdev);
	int ret;

	if (port < 0 || port >= HS104_MAX_CHANS)
		return -EINVAL;

	ret = regmap_update_bits(priv->regmap, HS104_REG_PW_EN,
				  HS104_EXECUTE | BIT(port), HS104_EXECUTE | BIT(port));
	if (ret)
		return ret;

	if (priv->port_enable_delay_ms)
		msleep(priv->port_enable_delay_ms);

	return 0;
}

static int hs104_pi_disable(struct pse_controller_dev *pcdev, int port)
{
	struct hs104_priv *priv = to_hs104(pcdev);

	if (port < 0 || port >= HS104_MAX_CHANS)
		return -EINVAL;

	return regmap_update_bits(priv->regmap, HS104_REG_PW_EN,
				  HS104_EXECUTE | BIT(port), HS104_EXECUTE);
}

static int
hs104_pi_get_admin_state(struct pse_controller_dev *pcdev, int port,
			 struct pse_admin_state *state)
{
	struct hs104_priv *priv = to_hs104(pcdev);
	unsigned int val;
	int ret;

	if (port < 0 || port >= HS104_MAX_CHANS)
		return -EINVAL;

	ret = regmap_read(priv->regmap, HS104_REG_PW_EN, &val);
	if (ret)
		return ret;

	bool enabled = val & BIT(port);

	state->c33_admin_state =
		enabled ?
		ETHTOOL_C33_PSE_ADMIN_STATE_ENABLED :
		ETHTOOL_C33_PSE_ADMIN_STATE_DISABLED;

	hs104_update_enabled_led(priv, port, enabled);

	return 0;
}

static int
hs104_pi_get_pw_status(struct pse_controller_dev *pcdev, int port,
		       struct pse_pw_status *status)
{
	struct hs104_priv *priv = to_hs104(pcdev);
	unsigned int val;
	int ret;

	if (port < 0 || port >= HS104_MAX_CHANS)
		return -EINVAL;

	ret = regmap_read(priv->regmap, HS104_REG_PW_STATUS, &val);
	if (ret)
		return ret;

	bool delivering = val & BIT(port);

	status->c33_pw_status = delivering ?
		ETHTOOL_C33_PSE_PW_D_STATUS_DELIVERING :
		ETHTOOL_C33_PSE_PW_D_STATUS_DISABLED;

	hs104_update_delivering_led(priv, port, delivering);

	return 0;
}

static int
hs104_pi_get_voltage(struct pse_controller_dev *pcdev,
		     int port, int *uV)
{
	struct hs104_priv *priv = to_hs104(pcdev);

	(void)port;

	*uV = hs104_read_be16(priv, HS104_REG_INPUT_V, HS104_uV_STEP);
	return (*uV < 0) ? *uV : 0;
}

static int
hs104_pi_get_current(struct pse_controller_dev *pcdev,
		     int port, int *uA)
{
	struct hs104_priv *priv = to_hs104(pcdev);

	if (port < 0 || port >= HS104_MAX_CHANS)
		return -EINVAL;

	*uA = hs104_read_be16(priv,
			      HS104_REG_PORT0_A + port * 2,
			      HS104_uA_STEP);
	return (*uA < 0) ? *uA : 0;
}

static int
hs104_pi_get_actual_pw(struct pse_controller_dev *pcdev,
		       int port, int *mW)
{
	struct hs104_priv *priv = to_hs104(pcdev);

	if (port < 0 || port >= HS104_MAX_CHANS)
		return -EINVAL;

	*mW = hs104_read_be16(priv,
			      HS104_REG_PORT0_POWER + port * 2,
			      HS104_mW_STEP);
	return (*mW < 0) ? *mW : 0;
}

static int
hs104_pi_get_pw_class(struct pse_controller_dev *pcdev, int port)
{
	struct hs104_priv *priv = to_hs104(pcdev);
	unsigned int val;
	int ret;

	if (port < 0 || port >= HS104_MAX_CHANS)
		return -EINVAL;

	ret = regmap_read(priv->regmap,
			  HS104_REG_PORT0_CLASS + port, &val);
	if (ret)
		return ret;

	return val;
}

static int
hs104_pi_get_pw_limit(struct pse_controller_dev *pcdev,
		      int port, int *max_mw)
{
	struct hs104_priv *priv = to_hs104(pcdev);
	unsigned int val, proto;
	int ret;

	if (port < 0 || port >= HS104_MAX_CHANS)
		return -EINVAL;

	ret = regmap_read(priv->regmap, HS104_REG_PROTOCOL, &val);
	if (ret)
		return ret;

	proto = (val >> (port * 2)) & HS104_PROTO_MASK;

	switch (proto) {
	case HS104_PROTO_AF:   *max_mw = HS104_AF_MW;   break;
	case HS104_PROTO_AT:   *max_mw = HS104_AT_MW;   break;
	case HS104_PROTO_BT:   *max_mw = HS104_BT_MW;   break;
	case HS104_PROTO_HIPO: *max_mw = HS104_HIPO_MW; break;
	default:
		return -ENODATA;
	}

	return 0;
}

static int
hs104_pi_set_pw_limit(struct pse_controller_dev *pcdev,
		      int port, int max_mw)
{
	struct hs104_priv *priv = to_hs104(pcdev);
	unsigned int proto, mask;

	if (port < 0 || port >= HS104_MAX_CHANS)
		return -EINVAL;

	if (max_mw <= HS104_AF_MW)
		proto = HS104_PROTO_AF;
	else if (max_mw <= HS104_AT_MW)
		proto = HS104_PROTO_AT;
	else if (max_mw <= HS104_BT_MW)
		proto = HS104_PROTO_BT;
	else if (max_mw <= HS104_HIPO_MW)
		proto = HS104_PROTO_HIPO;
	else
		return -EINVAL;

	mask = HS104_PROTO_MASK << (port * 2);
	proto <<= (port * 2);

	return regmap_update_bits(priv->regmap,
				  HS104_REG_PROTOCOL,
				  mask, proto);
}

static const struct ethtool_c33_pse_pw_limit_range hs104_pw_ranges[] = {
	{ .max = HS104_AF_MW },
	{ .max = HS104_AT_MW },
	{ .max = HS104_BT_MW },
	{ .max = HS104_HIPO_MW },
};

static int
hs104_pi_get_pw_limit_ranges(struct pse_controller_dev *pcdev, int port,
			     struct pse_pw_limit_ranges *ranges)
{
	ranges->c33_pw_limit_ranges = hs104_pw_ranges;
	return ARRAY_SIZE(hs104_pw_ranges);
}

static const struct pse_controller_ops hs104_ops = {
	.pi_enable			= hs104_pi_enable,
	.pi_disable			= hs104_pi_disable,
	.pi_get_admin_state		= hs104_pi_get_admin_state,
	.pi_get_pw_status		= hs104_pi_get_pw_status,
	.pi_get_voltage		= hs104_pi_get_voltage,
	.pi_get_current		= hs104_pi_get_current,
	.pi_get_actual_pw		= hs104_pi_get_actual_pw,
	.pi_get_pw_class		= hs104_pi_get_pw_class,
	.pi_get_pw_limit		= hs104_pi_get_pw_limit,
	.pi_set_pw_limit		= hs104_pi_set_pw_limit,
	.pi_get_pw_limit_ranges	= hs104_pi_get_pw_limit_ranges,
};

/* --- I2C --- */

static const struct regmap_config hs104_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
};

static int hs104_i2c_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct pse_controller_dev *pcdev;
	struct hs104_priv *priv;
	unsigned int val;
	int ret;

	pcdev = devm_kzalloc(dev, sizeof(*pcdev), GFP_KERNEL);
	if (!pcdev)
		return -ENOMEM;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	dev_set_drvdata(dev, priv);

	priv->regmap = devm_regmap_init_i2c(client,
					    &hs104_regmap_config);
	if (IS_ERR(priv->regmap))
		return PTR_ERR(priv->regmap);

	ret = regmap_read(priv->regmap, HS104_REG_DEVID, &val);
	if (ret)
		return ret;

	if ((val & 0xff) != HS104_DEVICE_ID)
		return -ENODEV;

	pcdev->ops = &hs104_ops;
	pcdev->nr_ports = HS104_MAX_CHANS;

	if (dev->of_node)
		of_property_read_u32(dev->of_node,
			     "hasivo,port-enable-delay-ms",
			     &priv->port_enable_delay_ms);

	for (int i = 0; i < HS104_MAX_CHANS; i++) {
		struct hs104_port_led *pled = &priv->port_led[i];

		pled->delivering = false;
		pled->delivering_trig.name = devm_kasprintf(dev, GFP_KERNEL,
						 "%s:port%d_delivering",
						 dev->of_node ? dev->of_node->name : dev_name(dev),
						 i);
		if (!pled->delivering_trig.name)
			return -ENOMEM;

		pled->enabled = false;
		pled->enabled_trig.name = devm_kasprintf(dev, GFP_KERNEL,
						 "%s:port%d_enabled",
						 dev->of_node ? dev->of_node->name : dev_name(dev),
						 i);
		if (!pled->enabled_trig.name)
			return -ENOMEM;

		ret = devm_led_trigger_register(dev, &pled->delivering_trig);
		if (ret)
			return ret;
	}

	return devm_pse_controller_register(dev, pcdev);
}

static const struct of_device_id hs104_of_match[] = {
	{ .compatible = "hasivo,hs104", },
	{ },
};
MODULE_DEVICE_TABLE(of, hs104_of_match);

static struct i2c_driver hs104_driver = {
	.probe = hs104_i2c_probe,
	.driver = {
		.name = "hasivo_hs104",
		.of_match_table = hs104_of_match,
	},
};
module_i2c_driver(hs104_driver);

MODULE_AUTHOR("Bevan Weiss <bevan.weiss@gmail.com>");
MODULE_DESCRIPTION("Hasivo HS104 PoE PSE Controller driver");
MODULE_LICENSE("GPL");
