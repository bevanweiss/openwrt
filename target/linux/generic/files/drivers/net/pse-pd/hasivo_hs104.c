// SPDX-License-Identifier: GPL-2.0-only
/*
 * Hasivo HS104 PoE PSE controller (I2C)
 *
 * Copyright (c) 2025 Bevan Weiss <bevan.weiss@gmail.com>
 *
 * The HS104PTI/HS104PBI are single-chip PoE PSE controllers managing 4
 * delivery channels. Allowing them to supply 4 ports of 802.3af/at/bt
 * power.
 * The HS104PTI can have 1x 802.3bt port and 3x 802.3at ports.
 * The HS104PBI can have 4x 802.3bt ports.
 */

#include <linux/bitfield.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/leds.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/pse-pd/pse.h>

#define HS104_MAX_PORTS		4

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

struct hs104_port_desc {
	struct led_trigger delivering_trig;
	struct led_trigger enabled_trig;
	bool delivering;
	bool enabled;
};

struct hs104_priv {
	struct i2c_client *client;
	struct pse_controller_dev pcdev;
	struct device_node *np;
	struct hs104_port_desc port[HS104_MAX_PORTS];
	unsigned int port_enable_delay_ms;
};

static struct hs104_priv *to_hs104(struct pse_controller_dev *pcdev)
{
	return container_of(pcdev, struct hs104_priv, pcdev);
}

static int hs104_read_be16(struct hs104_priv *priv,
			   unsigned int reg, unsigned int step)
{
	int ret;

	ret = i2c_smbus_read_word_data(priv->client, reg);
	if (ret)
		return ret;

	return (ret & 0x3fff) * step;
}

static void
hs104_update_led(struct led_trigger *trig, bool *state, bool new)
{
	if (*state == new)
		return;

	*state = new;

	if (new)
		led_trigger_event(trig, LED_FULL);
	else
		led_trigger_event(trig, LED_OFF);
}


/* --- PSE ops --- */
static int hs104_pi_enable(struct pse_controller_dev *pcdev, int port)
{
	struct hs104_priv *priv = to_hs104(pcdev);
	int ret, val;

	if (port < 0 || port >= HS104_MAX_PORTS)
		return -EINVAL;

	ret = i2c_smbus_read_byte_data(priv->client, HS104_REG_PW_EN);
	if (ret)
		return ret;

	val = ret | HS104_EXECUTE | BIT(port);

	ret = i2c_smbus_write_byte_data(priv->client, HS104_REG_PW_EN, val);
	if (ret)
		return ret;

	if (priv->port_enable_delay_ms)
		msleep(priv->port_enable_delay_ms);

	return 0;
}

static int hs104_pi_disable(struct pse_controller_dev *pcdev, int port)
{
	struct hs104_priv *priv = to_hs104(pcdev);
	int ret, val;

	if (port < 0 || port >= HS104_MAX_PORTS)
		return -EINVAL;

	ret = i2c_smbus_read_byte_data(priv->client, HS104_REG_PW_EN);
	if (ret)
		return ret;

	val = (ret & ~BIT(port)) | HS104_EXECUTE;

	ret = i2c_smbus_write_byte_data(priv->client, HS104_REG_PW_EN, val);
	if (ret)
		return ret;

	return 0;
}

static int
hs104_pi_get_admin_state(struct pse_controller_dev *pcdev, int port,
			 struct pse_admin_state *admin_state)
{
	struct hs104_priv *priv = to_hs104(pcdev);
	int ret;

	if (port < 0 || port >= HS104_MAX_PORTS)
		return -EINVAL;

	ret = i2c_smbus_read_byte_data(priv->client, HS104_REG_PW_EN);
	if (ret < 0) {
		admin_state->c33_admin_state =
			ETHTOOL_C33_PSE_ADMIN_STATE_UNKNOWN;
		return ret;
	}

	bool enabled = ret & BIT(port);

	admin_state->c33_admin_state =
		enabled ?
		ETHTOOL_C33_PSE_ADMIN_STATE_ENABLED :
		ETHTOOL_C33_PSE_ADMIN_STATE_DISABLED;

	hs104_update_led(&priv->port[port].enabled_trig,
		&priv->port[port].enabled,
		enabled);

	return 0;
}

static int
hs104_pi_get_pw_status(struct pse_controller_dev *pcdev, int port,
		       struct pse_pw_status *pw_status)
{
	struct hs104_priv *priv = to_hs104(pcdev);
	int ret;

	if (port < 0 || port >= HS104_MAX_PORTS)
		return -EINVAL;

	ret = i2c_smbus_read_byte_data(priv->client, HS104_REG_PW_STATUS);
	if (ret < 0) {
		pw_status->c33_pw_status = ETHTOOL_C33_PSE_PW_D_STATUS_UNKNOWN;
		return ret;
	}

	bool delivering = ret & BIT(port);
	pw_status->c33_pw_status = delivering ?
		ETHTOOL_C33_PSE_PW_D_STATUS_DELIVERING :
		ETHTOOL_C33_PSE_PW_D_STATUS_DISABLED;

	hs104_update_led(&priv->port[port].delivering_trig,
		&priv->port[port].delivering,
		delivering);

	return 0;
}

static int
hs104_pi_get_voltage(struct pse_controller_dev *pcdev,
		     int /*port*/, int *uV)
{
	struct hs104_priv *priv = to_hs104(pcdev);

	*uV = hs104_read_be16(priv, HS104_REG_INPUT_V, HS104_uV_STEP);
	return (*uV < 0) ? *uV : 0;
}

static int
hs104_pi_get_current(struct pse_controller_dev *pcdev,
		     int port, int *uA)
{
	struct hs104_priv *priv = to_hs104(pcdev);

	if (port < 0 || port >= HS104_MAX_PORTS)
		return -EINVAL;

	*uA = hs104_read_be16(priv,
			      HS104_REG_PORT0_A + 2*port,
			      HS104_uA_STEP);
	return (*uA < 0) ? *uA : 0;
}

static int
hs104_pi_get_actual_pw(struct pse_controller_dev *pcdev,
		       int port, int *mW)
{
	struct hs104_priv *priv = to_hs104(pcdev);

	if (port < 0 || port >= HS104_MAX_PORTS)
		return -EINVAL;

	*mW = hs104_read_be16(priv,
			      HS104_REG_PORT0_POWER + 2*port,
			      HS104_mW_STEP);
	return (*mW < 0) ? *mW : 0;
}

static int
hs104_pi_get_pw_class(struct pse_controller_dev *pcdev, int port)
{
	struct hs104_priv *priv = to_hs104(pcdev);
	int ret;

	if (port < 0 || port >= HS104_MAX_PORTS)
		return -EINVAL;

	ret = i2c_smbus_read_byte_data(priv->client,
				       HS104_REG_PORT0_CLASS + port);
	if (ret < 0)
		return ret;

	return ret;
}

static int
hs104_pi_get_pw_limit(struct pse_controller_dev *pcdev,
		      int port, int *max_mw)
{
	struct hs104_priv *priv = to_hs104(pcdev);
	unsigned int proto;
	int ret;

	if (port < 0 || port >= HS104_MAX_PORTS)
		return -EINVAL;

	ret = i2c_smbus_read_byte_data(priv->client, HS104_REG_PROTOCOL);
	if (ret < 0)
		return ret;

	proto = (ret >> (port * 2)) & HS104_PROTO_MASK;
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
	int proto, mask;
	int ret;

	if (port < 0 || port >= HS104_MAX_PORTS)
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
	proto = proto << (port * 2);

	ret = i2c_smbus_read_byte_data(priv->client, HS104_REG_PROTOCOL);
	if (ret < 0)
		return ret;

	ret = ret & ~mask;
	ret = ret | proto;

	return i2c_smbus_write_byte_data(priv->client, HS104_REG_PROTOCOL, ret);
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

static int hs104_i2c_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct hs104_priv *priv;
	int ret;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->client = client;

	ret = i2c_smbus_read_byte_data(client, HS104_REG_DEVID);
	if (ret < 0)
		return ret;

	if ((ret & 0xff) != HS104_DEVICE_ID) {
		dev_err(dev, "Wrong device ID: 0x%x\n", ret & 0xff);
		return -ENXIO;
	}

	if (dev->of_node)
		of_property_read_u32(dev->of_node, "hasivo,port-enable-delay-ms",
			     &priv->port_enable_delay_ms);

	for (int i = 0; i < HS104_MAX_PORTS; i++) {
		struct hs104_port_desc *pled = &priv->port[i];

		pled->delivering = false;
		pled->delivering_trig.name = devm_kasprintf(dev, GFP_KERNEL,
						 "%s:port%ddelivering",
						 dev->of_node ? dev->of_node->name : dev_name(dev),
						 i);
		if (!pled->delivering_trig.name)
			return -ENOMEM;

		ret = devm_led_trigger_register(dev, &pled->delivering_trig);
		if (ret)
			return ret;

		pled->enabled = false;
		pled->enabled_trig.name = devm_kasprintf(dev, GFP_KERNEL,
						 "%s:port%denabled",
						 dev->of_node ? dev->of_node->name : dev_name(dev),
						 i);
		if (!pled->enabled_trig.name)
			return -ENOMEM;

		ret = devm_led_trigger_register(dev, &pled->enabled_trig);
		if (ret)
			return ret;
	}

	priv->np = dev->of_node;
	priv->pcdev.owner = THIS_MODULE;
	priv->pcdev.ops = &hs104_ops;
	priv->pcdev.dev = dev;
	priv->pcdev.types = ETHTOOL_PSE_C33;
	priv->pcdev.nr_lines = HS104_MAX_PORTS;

	ret = devm_pse_controller_register(dev, &priv->pcdev);
	if (ret) {
		dev_err(dev, "Failed to register PSE controller: 0x%x\n", ret);
		return ret;
	}

	return 0;
}

static const struct of_device_id hs104_of_match[] = {
	{ .compatible = "hasivo,hs104pti", },
	{ .compatible = "hasivo,hs104pbi", },
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
