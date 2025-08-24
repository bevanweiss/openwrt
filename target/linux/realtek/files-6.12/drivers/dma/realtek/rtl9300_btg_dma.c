// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/regmap.h>
#include <linux/of_address.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/delay.h>

#define WRITE_CTRL_ADDR        0x144100
#define WRITE_ITER_ADDR        0x144104
#define WRITE_ADDR_ADDR        0x144110
#define WRITE_ADDR_MASK_ADDR   0x144114
#define WRITE_CTRL_ENABLE      BIT(31)

#define RTL9300_LX0_BTC_WRITE_IN_VEC_ADDR(idx) (0x144130 + ((idx) << 2)) // 16 entries

#define BTG_MAX_SG 16

struct rtl9300_btg_dma {
    struct device *dev;
    struct regmap *regmap;
    spinlock_t lock;
};

struct rtl9300_btg_desc {
    struct dma_async_tx_descriptor txd;
    struct scatterlist *sgl;
    unsigned int sg_len;
    dma_addr_t dst;
    size_t len;
    enum dma_transfer_direction dir;
};

static const struct regmap_config rtl9300_btg_regmap_config = {
    .reg_bits = 32,
    .val_bits = 32,
    .reg_stride = 4,
    .max_register = 0x200000, // Adjust as needed
};

static int rtl9300_btg_dma_sg_transfer(struct rtl9300_btg_dma *btg,
                                       struct scatterlist *sgl, unsigned int sg_len)
{
    unsigned int i;
    u32 ctrl;
    int timeout;

    if (sg_len > BTG_MAX_SG)
        return -EINVAL;

    // Program scatter-gather addresses
    for (i = 0; i < sg_len; ++i)
        regmap_write(btg->regmap, RTL9300_LX0_BTC_WRITE_IN_VEC_ADDR(i), sg_dma_address(&sgl[i]));

    // Set number of iterations (total bytes/words)
    regmap_write(btg->regmap, RTL9300_LX0_BTG_WRITE_ITER_ADDR, sg_len);

    // Enable BTG write
    regmap_write(btg->regmap, WRITE_CTRL_ADDR, RTL9300_LX0_BTG_WRITE_CTRL_ENABLE);

    // Poll for completion
    timeout = 1000000;
    do {
        regmap_read(btg->regmap, WRITE_CTRL_ADDR, &ctrl);
        if (!(ctrl & RTL9300_LX0_BTG_WRITE_CTRL_ENABLE))
            return 0;
        udelay(1);
    } while (--timeout);

    dev_err(btg->dev, "DMA SG timeout\n");
    return -ETIMEDOUT;
}

// DMA engine callbacks (simplified)
static dma_cookie_t rtl9300_btg_tx_submit(struct dma_async_tx_descriptor *tx)
{
    struct rtl9300_btg_desc *desc = container_of(tx, struct rtl9300_btg_desc, txd);
    struct rtl9300_btg_dma *btg = tx->chan->private;
    unsigned long flags;
    int ret;

    spin_lock_irqsave(&btg->lock, flags);
    ret = rtl9300_btg_dma_sg_transfer(btg, desc->sgl, desc->sg_len);
    spin_unlock_irqrestore(&btg->lock, flags);

    return dma_cookie_assign(tx);
}

static struct dma_async_tx_descriptor *
rtl9300_btg_prep_slave_sg(struct dma_chan *chan, struct scatterlist *sgl,
                          unsigned int sg_len, enum dma_transfer_direction dir,
                          unsigned long flags, void *context)
{
    struct rtl9300_btg_desc *desc;

    if (sg_len > BTG_MAX_SG)
        return NULL;

    desc = kzalloc(sizeof(*desc), GFP_ATOMIC);
    if (!desc)
        return NULL;

    desc->sgl = sgl;
    desc->sg_len = sg_len;
    desc->dir = dir;
    dma_async_tx_descriptor_init(&desc->txd, chan);
    desc->txd.tx_submit = rtl9300_btg_tx_submit;
    return &desc->txd;
}

static int rtl9300_btg_probe(struct platform_device *pdev)
{
    struct rtl9300_btg_dma *btg;
    struct resource *res;
    void __iomem *base;

    btg = devm_kzalloc(&pdev->dev, sizeof(*btg), GFP_KERNEL);
    if (!btg)
        return -ENOMEM;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    base = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(base))
        return PTR_ERR(base);

    btg->regmap = devm_regmap_init_mmio(&pdev->dev, base, &rtl9300_btg_regmap_config);
    if (IS_ERR(btg->regmap))
        return PTR_ERR(btg->regmap);

    btg->dev = &pdev->dev;
    spin_lock_init(&btg->lock);

    // Register as a DMA engine (simplified, not a full implementation)
    // You would fill in struct dma_device and register with dma_async_device_register()

    dev_info(&pdev->dev, "RTL9300 BTG DMA regmap SG driver loaded\n");
    platform_set_drvdata(pdev, btg);
    return 0;
}

static int rtl9300_btg_remove(struct platform_device *pdev)
{
    return 0;
}

static const struct of_device_id rtl9300_btg_of_match[] = {
    { .compatible = "realtek,rtl9300-btg-dma", },
    { }
};
MODULE_DEVICE_TABLE(of, rtl9300_btg_of_match);

static struct platform_driver rtl9300_btg_driver = {
    .probe = rtl9300_btg_probe,
    .remove = rtl9300_btg_remove,
    .driver = {
        .name = "rtl9300-btg-dma",
        .of_match_table = rtl9300_btg_of_match,
    },
};
module_platform_driver(rtl9300_btg_driver);

MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Realtek RTL9300 BTG DMA regmap SG driver (template)");
MODULE_LICENSE("GPL");