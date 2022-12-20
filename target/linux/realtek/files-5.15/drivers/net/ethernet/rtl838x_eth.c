// SPDX-License-Identifier: GPL-2.0-only
/* linux/drivers/net/ethernet/rtl838x_eth.c
 * Copyright (C) 2020 B. Koblitz
 */

#include <asm/mach-realtek/otto.h>
#include <linux/bitfield.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/etherdevice.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_net.h>
#include <linux/of_mdio.h>
#include <linux/module.h>
#include <linux/phylink.h>
#include <linux/pkt_sched.h>
#include <net/dsa.h>
#include <net/switchdev.h>
#include <asm/cacheflush.h>

#include "../dsa/rtl83xx/rtl838x.h"
#include "../dsa/rtl83xx/rtl839x.h"
#include "../dsa/rtl83xx/rtl930x.h"
#include "../dsa/rtl83xx/rtl931x.h"
#include "rtl838x_eth.h"

#define DMA_RING(r)                                                     BIT(r)

/* RTL838x series */
#define RTL838X_MAC_ADDR_CTRL_ALE_HI_REG                (0x6b04)
#define RTL838X_MAC_ADDR_CTRL_ALE_LO_REG                (0x6b08)

#define RTL838X_MAC_ADDR_CTRL_MAC_HI_REG                (0xa320)
#define RTL838X_MAC_ADDR_CTRL_MAC_LO_REG                (0xa324)

#define RTL838X_MAC_ADDR_CTRL_HI_REG                    (0xa9ec)
#define RTL838X_MAC_ADDR_CTRL_LO_REG                    (0xa9f0)

#define RTL838X_DMA_IF_RX_RING_MAX              8
#define RTL838X_DMA_IF_RX_RING_LEN              300
#define RTL838X_DMA_IF_RX_RING_ENTRIES          (RTL838X_DMA_IF_RX_RING_MAX * RTL838X_DMA_IF_RX_RING_LEN)

#define RTL838X_DMA_IF_RX_BASE_DESC_ADDR_CTRL_REG(r)    (0x9f00 + (((r) / 32) * 0x4))
#define RTL838X_DMA_IF_RX_BASE_DESC_ADDR_CTRL_MASK              GENMASK(31, 0)

#define RTL838X_DMA_IF_RX_CUR_DESC_ADDR_CTRL_REG(r)     (0x9f20 + (((r) / 32) * 0x4))
#define RTL838X_DMA_IF_RX_CUR_DESC_ADDR_CTRL_MASK               GENMASK(31, 0)

#define RTL838X_DMA_IF_TX_RING_MAX              2
#define RTL838X_DMA_IF_TX_RING_LEN              160
#define RTL839X_DMA_IF_TX_RING_ENTRIES          (RTL839X_DMA_IF_TX_RING_MAX * RTL839X_DMA_IF_TX_RING_LEN)

#define RTL838X_DMA_IF_TX_BASE_DESC_ADDR_CTRL_REG(r)    (0x9f40 + (((r) / 32) * 0x4))
#define RTL838X_DMA_IF_TX_BASE_DESC_ADDR_CTRL_MASK              GENMASK(31, 0)

#define RTL838X_DMA_IF_TX_CUR_DESC_ADDR_CTRL_REG(r)     (0x9f48 + (((r) / 32) * 0x4))
#define RTL838X_DMA_IF_TX_CUR_DESC_ADDR_CTRL_MASK               GENMASK(31, 0)

#define RTL838X_DMA_IF_INTR_MSK_REG                     (0x9f50)
/* Reserved                                                     31 - 20 */
#define RTL838X_DMA_IF_INTR_MSK_TX_ALL_DONE                     GENMASK(19, 18)
#define RTL838X_DMA_IF_INTR_MSK_TX_DONE                         GENMASK(17, 16)
#define RTL838X_DMA_IF_INTR_MSK_RX_DONE                         GENMASK(15, 8)
#define RTL838X_DMA_IF_INTR_MSK_RUNOUT                          GENMASK(7, 0)

#define RTL838X_DMA_IF_INTR_STS_REG                     (0x9f54)
/* Reserved                                                     31 - 20 */
#define RTL838X_DMA_IF_INTR_STS_TX_ALL_DONE                     GENMASK(19, 18)
#define RTL838X_DMA_IF_INTR_STS_TX_DONE                         GENMASK(17, 16)
#define RTL838X_DMA_IF_INTR_STS_RX_DONE                         GENMASK(15, 8)
#define RTL838X_DMA_IF_INTR_STS_RUNOUT                          GENMASK(7, 0)

#define RTL838X_DMA_IF_CTRL_REG                         (0x9f58)
/* Reserved                                                     31 - 30 */
#define RTL838X_DMA_IF_CTRL_RX_TRUNCATE_LEN                     GENMASK(29, 16)
/* Reserved                                                     15 - 6 */
#define RTL838X_DMA_IF_CTRL_TX_PAD_EN                           BIT(5)
#define RTL838X_DMA_IF_CTRL_RX_TRUNCATE_EN                      BIT(4)
#define RTL838X_DMA_IF_CTRL_TX_EN                               BIT(3)
#define RTL838X_DMA_IF_CTRL_RX_EN                               BIT(2)
#define RTL838X_DMA_IF_CTRL_TX_FETCH                            BIT(1)
#define RTL838X_DMA_IF_CTRL_TX_BUSY                             BIT(0)

#define RTL838X_DMA_IF_RX_RING_SIZE_REG(r)              (0xb7e4 + (((r) / 8) * 0x4))
#define _RTL838X_DMA_IF_RX_RING_SIZE_MASK                       GENMASK(3, 0)
#define RTL838X_DMA_IF_RX_RING_SIZE_GET(reg, ring) \
        (((reg) >> (((ring) % 8) * 4)) & _RTL838X_DMA_IF_RX_RING_SIZE_MASK)
#define RTL838X_DMA_IF_RX_RING_SIZE_SET(r, s) \
        (((s) & _RTL838X_DMA_IF_RX_RING_SIZE_MASK) << (((r) % 8) * 4))

#define RTL838X_DMA_IF_RX_RING_CNTR_REG(r)              (0xb7e8 + (((r) / 8) * 0x4))
#define _RTL838X_DMA_IF_RX_RING_CNTR_MASK                       GENMASK(3, 0)
#define RTL838X_DMA_IF_RX_RING_CNTR_GET(reg, ring) \
        (((reg) >> (((ring) % 8) * 4)) & _RTL838X_DMA_IF_RX_RING_CNTR_MASK)
#define RTL838X_DMA_IF_RX_RING_CNTR_SET(r, c) \
        (((c) & _RTL838X_DMA_IF_RX_RING_CNTR_MASK) << (((r) % 8) * 4))

/* RTL839x series */
#define RTL839X_MAC_ADDR_CTRL_HI_REG                    (0x02b4)
#define RTL839X_MAC_ADDR_CTRL_LO_REG                    (0x02b8)

#define RTL839X_DMA_IF_RX_RING_SIZE_REG(r)              (0x6038 + (((r) / 8) * 0x4))
#define _RTL839X_DMA_IF_RX_RING_SIZE_MASK                       GENMASK(3, 0)
#define RTL839X_DMA_IF_RX_RING_SIZE_GET(reg, ring) \
        (((reg) >> (((ring) % 8) * 4)) & _RTL839X_DMA_IF_RX_RING_SIZE_MASK)
#define RTL839X_DMA_IF_RX_RING_SIZE_SET(r, s) \
        (((s) & _RTL839X_DMA_IF_RX_RING_SIZE_MASK) << (((r) % 8) * 4))

#define RTL839X_DMA_IF_RX_RING_CNTR_REG(r)              (0x603c + (((r) / 8) * 0x4))
#define _RTL839X_DMA_IF_RX_RING_CNTR_MASK                       GENMASK(3, 0)
#define RTL839X_DMA_IF_RX_RING_CNTR_GET(reg, ring) \
        (((reg) >> (((ring) % 8) * 4)) & _RTL839X_DMA_IF_RX_RING_CNTR_MASK)
#define RTL839X_DMA_IF_RX_RING_CNTR_SET(r, c) \
        (((c) & _RTL839X_DMA_IF_RX_RING_CNTR_MASK) << (((r) % 8) * 4))

#define RTL839X_DMA_IF_RX_RING_MAX              8
#define RTL839X_DMA_IF_RX_RING_LEN              300
#define RTL839X_DMA_IF_RX_RING_ENTRIES          (RTL839X_DMA_IF_RX_RING_MAX * RTL839X_DMA_IF_RX_RING_LEN)

#define RTL839X_DMA_IF_RX_BASE_DESC_ADDR_CTRL_REG(r)    (0x780c + (((r) / 32) * 0x4))
#define RTL839X_DMA_IF_RX_BASE_DESC_ADDR_CTRL_MASK              GENMASK(31, 0)

#define RTL839X_DMA_IF_RX_CUR_DESC_ADDR_CTRL_REG(r)     (0x782c + (((r) / 32) * 0x4))
#define RTL839X_DMA_IF_RX_CUR_DESC_ADDR_CTRL_MASK               GENMASK(31, 0)

#define RTL839X_DMA_IF_TX_RING_MAX              2
#define RTL839X_DMA_IF_TX_RING_LEN              160
#define RTL839X_DMA_IF_TX_RING_ENTRIES          (RTL839X_DMA_IF_TX_RING_MAX * RTL839X_DMA_IF_TX_RING_LEN)

#define RTL839X_DMA_IF_TX_BASE_DESC_ADDR_CTRL_REG(r)    (0x784c + (((r) / 32) * 0x4))
#define RTL839X_DMA_IF_TX_BASE_DESC_ADDR_CTRL_MASK              GENMASK(31, 0)

#define RTL839X_DMA_IF_TX_CUR_DESC_ADDR_CTRL_REG(r)     (0x7854 + (((r) / 32) * 0x4))
#define RTL839X_DMA_IF_TX_CUR_DESC_ADDR_CTRL_MASK               GENMASK(31, 0)

#define RTL839X_DMA_IF_INTR_MSK_REG                     (0x7864)
/* Reserved                                                     31 - 23 */
#define RTL839X_DMA_IF_INTR_MSK_NTFY_DONE                       BIT(22)
#define RTL839X_DMA_IF_INTR_MSK_NTFY_BF_RUNOUT                  BIT(21)
#define RTL839X_DMA_IF_INTR_MSK_LOCAL_NTFY_BUF_RUNOUT           BIT(20)
#define RTL839X_DMA_IF_INTR_MSK_TX_ALL_DONE                     GENMASK(19, 18)
#define RTL839X_DMA_IF_INTR_MSK_TX_DONE                         GENMASK(17, 16)
#define RTL839X_DMA_IF_INTR_MSK_RX_DONE                         GENMASK(16, 8)
#define RTL839X_DMA_IF_INTR_MSK_RX_RUNOUT                       GENMASK(7, 0)

#define RTL839X_DMA_IF_INTR_STS_REG                     (0x7868)
/* Reserved                                                     31 - 23 */
#define RTL839X_DMA_IF_INTR_STS_NTFY_DONE                       BIT(22)
#define RTL839X_DMA_IF_INTR_STS_NTFY_BF_RUNOUT                  BIT(21)
#define RTL839X_DMA_IF_INTR_STS_LOCAL_NTFY_BUF_RUNOUT           BIT(20)
#define RTL839X_DMA_IF_INTR_STS_TX_ALL_DONE                     GENMASK(19, 18)
#define RTL839X_DMA_IF_INTR_STS_TX_DONE                         GENMASK(17, 16)
#define RTL839X_DMA_IF_INTR_STS_RX_DONE                         GENMASK(16, 8)
#define RTL839X_DMA_IF_INTR_STS_RX_RUNOUT                       GENMASK(7, 0)

#define RTL839X_DMA_IF_CTRL_REG                         (0x786c)
/* Reserved                                                     31 - 19 */
#define RTL839X_DMA_IF_CTRL_RX_TRUNCATE_LEN                     GENMASK(18, 5)
#define RTL839X_DMA_IF_CTRL_RX_TRUNCATE_EN                      BIT(4)
#define RTL839X_DMA_IF_CTRL_TX_EN                               BIT(3)
#define RTL839X_DMA_IF_CTRL_RX_EN                               BIT(2)
#define RTL839X_DMA_IF_CTRL_TX_FETCH                            BIT(1)
#define RTL839X_DMA_IF_CTRL_TX_BUSY                             BIT(0)

/* RTL930x series */
#define RTL930X_DMA_IF_RX_RING_SIZE_REG(r)              (0x7c60 + (((r) / 3) * 0x4))
#define _RTL930X_DMA_IF_RX_RING_SIZE_MASK                       GENMASK(9, 0)
#define RTL930X_DMA_IF_RX_RING_SIZE_GET(reg, ring) \
        (((reg) >> (((ring) % 3) * 10)) & _RTL930X_DMA_IF_RX_RING_SIZE_MASK)
#define RTL930X_DMA_IF_RX_RING_SIZE_SET(r, s) \
        (((s) & _RTL930X_DMA_IF_RX_RING_SIZE_MASK) << (((r) % 3) * 10))

#define RTL930X_DMA_IF_RX_RING_CNTR_REG(r)              (0x7c8c + (((r) / 3) * 0x4))
#define _RTL930X_DMA_IF_RX_RING_CNTR_MASK                       GENMASK(9, 0)
#define RTL930X_DMA_IF_RX_RING_CNTR_GET(reg, ring) \
        (((reg) >> (((ring) % 3)) * 10) & _RTL930X_DMA_IF_RX_RING_CNTR_MASK)
#define RTL930X_DMA_IF_RX_RING_CNTR_SET(r, c) \
        (((c) & _RTL930X_DMA_IF_RX_RING_CNTR_MASK) << (((r) % 3) * 10))

#define RTL930X_L2_TBL_FLUSH_CTRL1_REG                  (0x9404)
/* Reserved                                                     31 - 22 */
#define RTL930X_L2_TBL_FLUSH_CTRL1_PORT_ID                      GENMASK(21 ,11)
#define RTL930X_L2_TBL_FLUSH_CTRL1_REPLACING_PORT_ID            GENMASK(10, 0)

#define RTL930X_L2_TBL_FLUSH_CTRL2_REG                  (0x9408)
/* Reserved                                                     31 */
#define RTL930X_L2_TBL_FLUSH_CTRL2_STS                          BIT(30)
#define RTL930X_L2_TBL_FLUSH_CTRL2_ACT                          BIT(29)
#define RTL930X_L2_TBL_FLUSH_CTRL2_FVID_CMP                     BIT(28)
#define RTL930X_L2_TBL_FLUSH_CTRL2_AGG_VID_CMP                  BIT(27)
#define RTL930X_L2_TBL_FLUSH_CTRL2_PORT_CMP                     BIT(26)
#define RTL930X_L2_TBL_FLUSH_CTRL2_ENTRY_TYPE                   GENMASK(25, 24)
#define RTL930X_L2_TBL_FLUSH_CTRL2_FVID                         GENMASK(23, 12)
#define RTL930X_L2_TBL_FLUSH_CTRL2_AGG_VID                      GENMASK(11, 0)

#define RTL930X_MAC_L2_ADDR_CTRL_HI_REG                 (0xc714)
#define RTL930X_MAC_L2_ADDR_CTRL_LO_REG                 (0xc718)

#define RTL930X_DMA_IF_RX_RING_MAX              32
#define RTL930X_DMA_IF_RX_RING_LEN              300
#define RTL930X_DMA_IF_RX_RING_ENTRIES          (RTL930X_DMA_IF_RX_RING_MAX * RTL930X_DMA_IF_RX_RING_LEN)

#define RTL930X_DMA_IF_RX_BASE_DESC_ADDR_CTRL_REG(r)    (0xdf00 + (((r) / 32) * 0x4))
#define RTL930X_DMA_IF_RX_BASE_DESC_ADDR_CTRL_MASK              GENMASK(31, 0)

#define RTL930X_DMA_IF_RX_CUR_DESC_ADDR_CTRL_REG(r)     (0xdf80 + (((r) / 32) * 0x4))
#define RTL930X_DMA_IF_RX_CUR_DESC_ADDR_CTRL_MASK               GENMASK(31, 0)

#define RTL930X_DMA_IF_TX_RING_MAX              2
#define RTL930X_DMA_IF_TX_RING_LEN              160
#define RTL930X_DMA_IF_TX_RING_ENTRIES          (RTL930X_DMA_IF_TX_RING_MAX * RTL930X_DMA_IF_TX_RING_LEN)

#define RTL930X_DMA_IF_TX_BASE_DESC_ADDR_CTRL_REG(r)    (0xe000 + (((r) / 32) * 0x4))
#define RTL930X_DMA_IF_TX_BASE_DESC_ADDR_CTRL_MASK              GENMASK(31, 0)

#define RTL930X_DMA_IF_TX_CUR_DESC_ADDR_CTRL_REG(r)     (0xe008 + (((r) / 32) * 0x4))
#define RTL930X_DMA_IF_TX_CUR_DESC_ADDR_CTRL_MASK               GENMASK(31, 0)

#define RTL930X_DMA_IF_INTR_RX_RUNOUT_MSK_REG           (0xe010)
#define RTL930X_DMA_IF_INTR_RX_RUNOUT_MSK_DONE                  GENMASK(31, 0)

#define RTL930X_DMA_IF_INTR_RX_DONE_MSK_REG             (0xe014)
#define RTL930X_DMA_IF_INTR_RX_DONE_MSK_DONE                    GENMASK(31, 0)

#define RTL930X_DMA_IF_INTR_TX_DONE_MSK_REG             (0xe018)
/* Reserved                                                     31 - 4 */
#define RTL930X_DMA_IF_INTR_TX_DONE_MSK_ALL_DONE                GENMASK(3, 2)
#define RTL930X_DMA_IF_INTR_TX_DONE_MSK_DONE                    GENMASK(1, 0)

#define RTL930X_DMA_IF_INTR_RX_RUNOUT_STS_REG           (0xe01c)
#define RTL930X_DMA_IF_INTR_RX_RUNOUT_STS_DONE                  GENMASK(31, 0)

#define RTL930X_DMA_IF_INTR_RX_DONE_STS_REG             (0xe020)
#define RTL930X_DMA_IF_INTR_RX_DONE_STS_DONE                    GENMASK(31, 0)

#define RTL930X_DMA_IF_INTR_TX_DONE_STS_REG             (0xe024)
/* Reserved                                                     31 - 4 */
#define RTL930X_DMA_IF_INTR_TX_DONE_STS_ALL_DONE                GENMASK(3, 2)
#define RTL930X_DMA_IF_INTR_TX_DONE_STS_DONE                    GENMASK(1, 0)

#define RTL930X_DMA_IF_CTRL_REG                         (0xe028)
/* Reserved                                                     31 - 30 */
#define RTL930X_DMA_IF_CTRL_RX_TRUNCATE_LEN                     GENMASK(29, 16)
/* Reserved                                                     15 - 7 */
#define RTL930X_DMA_IF_CTRL_RX_TRUNCATE_EN                      BIT(6)
#define RTL930X_DMA_IF_CTRL_TX_EN                               BIT(5)
#define RTL930X_DMA_IF_CTRL_RX_EN                               BIT(4)
#define RTL930X_DMA_IF_CTRL_TX_HIGH_FETCH                       BIT(3)
#define RTL930X_DMA_IF_CTRL_TX_LOW_FETCH                        BIT(2)
#define RTL930X_DMA_IF_CTRL_TX_HIGH_BUSY                        BIT(1)
#define RTL930X_DMA_IF_CTRL_TX_LOW_BUSY                         BIT(0)

/* RTL931x series */

#define RTL931X_MDX_CTRL_RSVD_REG                       (0x0fcc)
/* Reserved                                                     31 - 1 */
#define RTL931X_MDX_CTRL_RSVD_ESD_AUTO_RECOVERY                 BIT(0)

#define RTL931X_DMA_IF_RX_RING_MAX              32
#define RTL931X_DMA_IF_RX_RING_LEN              300
#define RTL931X_DMA_IF_RX_RING_ENTRIES          (RTL931X_DMA_IF_RX_RING_MAX * RTL931X_DMA_IF_RX_RING_LEN)

#define RTL931X_DMA_IF_RX_BASE_DESC_ADDR_CTRL_REG(r)    (0x0800 + (((r) / 32) * 0x4))
#define RTL931X_DMA_IF_RX_BASE_DESC_ADDR_CTRL_MASK              GENMASK(31, 0)

#define RTL931X_DMA_IF_RX_CUR_DESC_ADDR_CTRL_REG(r)     (0x0880 + (((r) / 32) * 0x4))
#define RTL931X_DMA_IF_RX_CUR_DESC_ADDR_CTRL_MASK               GENMASK(31, 0)

#define RTL931X_DMA_IF_INTR_RX_RUNOUT_MSK_REG           (0x0910)
#define RTL931X_DMA_IF_INTR_RX_RUNOUT_MSK_DONE                  GENMASK(31, 0)

#define RTL931X_DMA_IF_INTR_RX_DONE_MSK_REG             (0x0914)
#define RTL931X_DMA_IF_INTR_RX_DONE_MSK_DONE                    GENMASK(31, 0)

#define RTL931X_DMA_IF_INTR_TX_DONE_MSK_REG             (0x0918)
/* Reserved                                                     31 - 4 */
#define RTL931X_DMA_IF_INTR_TX_DONE_MSK_ALL_DONE                GENMASK(3, 2)
#define RTL931X_DMA_IF_INTR_TX_DONE_MSK_DONE                    GENMASK(1, 0)

#define RTL931X_DMA_IF_INTR_RX_RUNOUT_STS_REG           (0x091c)
#define RTL931X_DMA_IF_INTR_RX_RUNOUT_STS_DONE                  GENMASK(31, 0)

#define RTL931X_DMA_IF_INTR_RX_DONE_STS_REG             (0x0920)
#define RTL931X_DMA_IF_INTR_RX_DONE_STS_DONE                    GENMASK(31, 0)

#define RTL931X_DMA_IF_INTR_TX_DONE_STS_REG             (0x0924)
/* Reserved                                                     31 - 4 */
#define RTL931X_DMA_IF_INTR_TX_DONE_STS_ALL_DONE                GENMASK(3, 2)
#define RTL931X_DMA_IF_INTR_TX_DONE_STS_DONE                    GENMASK(1, 0)

#define RTL931X_DMA_IF_CTRL_REG                         (0x0928)
/* Reserved                                                     31 - 30 */
#define RTL931X_DMA_IF_CTRL_RX_TRUNCATE_LEN                     GENMASK(29, 16)
/* Reserved                                                     15 - 7 */
#define RTL931X_DMA_IF_CTRL_RX_TRUNCATE_EN                      BIT(6)
#define RTL931X_DMA_IF_CTRL_TX_EN                               BIT(5)
#define RTL931X_DMA_IF_CTRL_RX_EN                               BIT(4)
#define RTL931X_DMA_IF_CTRL_TX_HIGH_FETCH                       BIT(3)
#define RTL931X_DMA_IF_CTRL_TX_LOW_FETCH                        BIT(2)
#define RTL931X_DMA_IF_CTRL_TX_HIGH_BUSY                        BIT(1)
#define RTL931X_DMA_IF_CTRL_TX_LOW_BUSY                         BIT(0)

#define RTL931X_MAC_L2_ADDR_CTRL_HI_REG                 (0x135c)
#define RTL931X_MAC_L2_ADDR_CTRL_LO_REG                 (0x1360)

#define RTL931X_DMA_IF_RX_RING_SIZE_REG(r)              (0x2080 + (((r) / 3) * 0x4))
#define _RTL931X_DMA_IF_RX_RING_SIZE_MASK                       GENMASK(9, 0)
#define RTL931X_DMA_IF_RX_RING_SIZE_GET(reg, ring) \
        (((reg) >> (((ring) % 3) * 10)) & _RTL931X_DMA_IF_RX_RING_SIZE_MASK)
#define RTL931X_DMA_IF_RX_RING_SIZE_SET(r, s) \
        (((s) & _RTL931X_DMA_IF_RX_RING_SIZE_MASK) << (((r) % 3) * 10))

#define RTL931X_DMA_IF_RX_RING_CNTR_REG(r)              (0x20ac + (((r) / 3) * 0x4))
#define _RTL931X_DMA_IF_RX_RING_CNTR_MASK                       GENMASK(9, 0)
#define RTL931X_DMA_IF_RX_RING_CNTR_GET(reg, ring) \
        (((reg) >> (((ring) % 3) * 10)) & _RTL931X_DMA_IF_RX_RING_CNTR_MASK)
#define RTL931X_DMA_IF_RX_RING_CNTR_SET(r, c) \
        (((c) & _RTL931X_DMA_IF_RX_RING_CNTR_MASK) << (((r) % 3) * 10))

#define RTL931X_MEM_ACL_INIT_REG                        (0x40bc)
/* Reserved                                                     31 - 1 */
#define RTL931X_MEM_ACL_INIT_MEM_INIT                           BIT(0)

#define RTL931X_MEM_ENCAP_INIT_REG                      (0x4854)
/* Reserved                                                     31 - 1 */
#define RTL931X_MEM_ENCAP_INIT_MEM_INIT                         BIT(0)

#define RTL931X_MEM_MIB_INIT_REG                        (0x7e18)
/* Reserved                                                     31 - 1 */
#define RTL931X_MEM_MIB_INIT_MEM_RST                            BIT(0)

#define RTL931X_MEM_ALE_INIT_REG(p)                     (0x83f0 + (((p) / 32) * 0x4))

#define RTL931X_MEM_RALE_INIT_REG                       (0x82e4)
#define RLT931X_MEM_RALE_INIT_MASK                              GENMASK(10, 0)

#define RTL931X_DMA_IF_TX_RING_MAX              2
#define RTL931X_DMA_IF_TX_RING_LEN              160
#define RTL931X_DMA_IF_TX_RING_ENTRIES          (RTL931X_DMA_IF_TX_RING_MAX * RTL931X_DMA_IF_TX_RING_LEN)

#define RTL931X_DMA_IF_TX_BASE_DESC_ADDR_CTRL_REG(r)    (0x9000 + (((r) / 32) * 0x4))
#define RTL931X_DMA_IF_TX_BASE_DESC_ADDR_CTRL_MASK              GENMASK(31, 0)

#define RTL931X_DMA_IF_TX_CUR_DESC_ADDR_CTRL_REG(r)     (0x9008 + (((r) / 32) * 0x4))
#define RTL931X_DMA_IF_TX_CUR_DESC_ADDR_CTRL_MASK               GENMASK(31, 0)

int rtl838x_dma_if_rx_ring_size(const int ring)
{
	return RTL838X_DMA_IF_RX_RING_SIZE_REG(ring);
}

int rtl839x_dma_if_rx_ring_size(const int ring)
{
	return RTL839X_DMA_IF_RX_RING_SIZE_REG(ring);
}

int rtl930x_dma_if_rx_ring_size(const int ring)
{
	return RTL930X_DMA_IF_RX_RING_SIZE_REG(ring);
}

int rtl931x_dma_if_rx_ring_size(const int ring)
{
	return RTL931X_DMA_IF_RX_RING_SIZE_REG(ring);
}

int rtl838x_dma_if_rx_ring_cntr(const int ring)
{
	return RTL838X_DMA_IF_RX_RING_CNTR_REG(ring);
}

int rtl839x_dma_if_rx_ring_cntr(const int ring)
{
	return RTL839X_DMA_IF_RX_RING_CNTR_REG(ring);
}

int rtl930x_dma_if_rx_ring_cntr(const int ring)
{
	return RTL930X_DMA_IF_RX_RING_CNTR_REG(ring);
}

int rtl931x_dma_if_rx_ring_cntr(const int ring)
{
	return RTL931X_DMA_IF_RX_RING_CNTR_REG(ring);
}

extern struct rtl83xx_soc_info soc_info;

/* Maximum number of RX rings is 8 on RTL83XX and 32 on the 93XX
 * The ring is assigned by switch based on packet/port priortity
 * Maximum number of TX rings is 2, Ring 2 being the high priority
 * ring on the RTL93xx SoCs. RTL930X_DMA_IF_RX_RING_LEN gives the maximum length
 * for an RX ring, MAX_ENTRIES the maximum number of entries
 * available in total for all queues.
 */
#define MAX_RXRINGS	32
#define MAX_RXLEN	300
#define MAX_ENTRIES	(300 * 8)
#define TXRINGS		2
#define TXRINGLEN	160
#define NOTIFY_EVENTS	10
#define NOTIFY_BLOCKS	10
#define TX_EN		0x8
#define RX_EN		0x4
#define TX_EN_93XX	0x20
#define RX_EN_93XX	0x10
#define TX_DO		0x2
#define WRAP		0x2
#define MAX_PORTS	57
#define MAX_SMI_BUSSES	4

#define RING_BUFFER	1600

struct p_hdr {
	uint8_t		*buf;
	uint16_t	reserved;
	uint16_t	size;		/* buffer size */
	uint16_t	offset;
	uint16_t	len;		/* pkt len */
	/* cpu_tag[0] is a reserved uint16_t on RTL83xx */
	uint16_t	cpu_tag[10];
} __packed __aligned(1);

struct n_event {
	uint32_t	type:2;
	uint32_t	fidVid:12;
	uint64_t	mac:48;
	uint32_t	slp:6;
	uint32_t	valid:1;
	uint32_t	reserved:27;
} __packed __aligned(1);

struct ring_b {
	uint32_t	rx_r[RTL930X_DMA_IF_RX_RING_MAX][RTL930X_DMA_IF_RX_RING_LEN];
	uint32_t	tx_r[RTL838X_DMA_IF_TX_RING_MAX][RTL838X_DMA_IF_TX_RING_LEN];
	struct	p_hdr	rx_header[RTL930X_DMA_IF_RX_RING_MAX][RTL930X_DMA_IF_RX_RING_LEN];
	struct	p_hdr	tx_header[RTL838X_DMA_IF_TX_RING_MAX][RTL838X_DMA_IF_TX_RING_LEN];
	uint32_t	c_rx[RTL930X_DMA_IF_RX_RING_MAX];
	uint32_t	c_tx[RTL838X_DMA_IF_TX_RING_MAX];
	uint8_t		tx_space[RTL838X_DMA_IF_TX_RING_MAX * RTL838X_DMA_IF_TX_RING_LEN * RING_BUFFER];
	uint8_t		*rx_space;
};

struct notify_block {
	struct n_event	events[NOTIFY_EVENTS];
};

struct notify_b {
	struct notify_block	blocks[NOTIFY_BLOCKS];
	u32			reserved1[8];
	u32			ring[NOTIFY_BLOCKS];
	u32			reserved2[8];
};

static void rtl838x_create_tx_header(struct p_hdr *h, unsigned int dest_port, int prio)
{
	/* cpu_tag[0] is reserved on the RTL83XX SoCs */
	h->cpu_tag[1] = 0x0400;  /* BIT 10: RTL8380_CPU_TAG */
	h->cpu_tag[2] = 0x0200;  /* Set only AS_DPM, to enable DPM settings below */
	h->cpu_tag[3] = 0x0000;
	h->cpu_tag[4] = BIT(dest_port) >> 16;
	h->cpu_tag[5] = BIT(dest_port) & 0xffff;

	/* Set internal priority (PRI) and enable (AS_PRI) */
	if (prio >= 0)
		h->cpu_tag[2] |= ((prio & 0x7) | BIT(3)) << 12;
}

static void rtl839x_create_tx_header(struct p_hdr *h, unsigned int dest_port, int prio)
{
	/* cpu_tag[0] is reserved on the RTL83XX SoCs */
	h->cpu_tag[1] = 0x0100; /* RTL8390_CPU_TAG marker */
	h->cpu_tag[2] = BIT(4); /* AS_DPM flag */
	h->cpu_tag[3] = h->cpu_tag[4] = h->cpu_tag[5] = 0;
	/* h->cpu_tag[1] |= BIT(1) | BIT(0); */ /* Bypass filter 1/2 */
	if (dest_port >= 32) {
		dest_port -= 32;
		h->cpu_tag[2] |= (BIT(dest_port) >> 16) & 0xf;
		h->cpu_tag[3] = BIT(dest_port) & 0xffff;
	} else {
		h->cpu_tag[4] = BIT(dest_port) >> 16;
		h->cpu_tag[5] = BIT(dest_port) & 0xffff;
	}

	/* Set internal priority (PRI) and enable (AS_PRI) */
	if (prio >= 0)
		h->cpu_tag[2] |= ((prio & 0x7) | BIT(3)) << 8;
}

static void rtl930x_create_tx_header(struct p_hdr *h, unsigned int dest_port, int prio)
{
	h->cpu_tag[0] = 0x8000;  /* CPU tag marker */
	h->cpu_tag[1] = h->cpu_tag[2] = 0;
	h->cpu_tag[3] = 0;
	h->cpu_tag[4] = 0;
	h->cpu_tag[5] = 0;
	h->cpu_tag[6] = BIT(dest_port) >> 16;
	h->cpu_tag[7] = BIT(dest_port) & 0xffff;

	/* Enable (AS_QID) and set priority queue (QID) */
	if (prio >= 0)
		h->cpu_tag[2] = (BIT(5) | (prio & 0x1f)) << 8;
}

static void rtl931x_create_tx_header(struct p_hdr *h, unsigned int dest_port, int prio)
{
	h->cpu_tag[0] = 0x8000;  /* CPU tag marker */
	h->cpu_tag[1] = h->cpu_tag[2] = 0;
	h->cpu_tag[3] = 0;
	h->cpu_tag[4] = h->cpu_tag[5] = h->cpu_tag[6] = h->cpu_tag[7] = 0;
	if (dest_port >= 32) {
		dest_port -= 32;
		h->cpu_tag[4] = BIT(dest_port) >> 16;
		h->cpu_tag[5] = BIT(dest_port) & 0xffff;
	} else {
		h->cpu_tag[6] = BIT(dest_port) >> 16;
		h->cpu_tag[7] = BIT(dest_port) & 0xffff;
	}

	/* Enable (AS_QID) and set priority queue (QID) */
	if (prio >= 0)
		h->cpu_tag[2] = (BIT(5) | (prio & 0x1f)) << 8;
}

static void rtl93xx_header_vlan_set(struct p_hdr *h, int vlan)
{
	h->cpu_tag[2] |= BIT(4); /* Enable VLAN forwarding offload */
	h->cpu_tag[2] |= (vlan >> 8) & 0xf;
	h->cpu_tag[3] |= (vlan & 0xff) << 8;
}

struct rtl838x_rx_q {
	int id;
	struct rtl838x_eth_priv *priv;
	struct napi_struct napi;
};

struct rtl838x_eth_priv {
	struct net_device *netdev;
	struct platform_device *pdev;
	void *membase;
	spinlock_t lock;
	struct mii_bus *mii_bus;
	struct rtl838x_rx_q rx_qs[RTL930X_DMA_IF_RX_RING_MAX];
	struct phylink *phylink;
	struct phylink_config phylink_config;
	u16 id;
	u16 family_id;
	const struct rtl838x_eth_reg *r;
	u8 cpu_port;
	u32 lastEvent;
	u16 rxrings;
	u16 rxringlen;
	u32 smi_bus[MAX_PORTS];
	u8 smi_addr[MAX_PORTS];
	u32 sds_id[MAX_PORTS];
	bool smi_bus_isc45[MAX_SMI_BUSSES];
	bool phy_is_internal[MAX_PORTS];
	phy_interface_t interfaces[MAX_PORTS];
};

extern int rtl838x_phy_init(struct rtl838x_eth_priv *priv);
extern int rtl838x_read_sds_phy(int phy_addr, int phy_reg);
extern int rtl839x_read_sds_phy(int phy_addr, int phy_reg);
extern int rtl839x_write_sds_phy(int phy_addr, int phy_reg, u16 v);
extern int rtl930x_read_sds_phy(int phy_addr, int page, int phy_reg);
extern int rtl930x_write_sds_phy(int phy_addr, int page, int phy_reg, u16 v);
extern int rtl931x_read_sds_phy(int phy_addr, int page, int phy_reg);
extern int rtl931x_write_sds_phy(int phy_addr, int page, int phy_reg, u16 v);
extern int rtl930x_read_mmd_phy(u32 port, u32 devnum, u32 regnum, u32 *val);
extern int rtl930x_write_mmd_phy(u32 port, u32 devnum, u32 regnum, u32 val);
extern int rtl931x_read_mmd_phy(u32 port, u32 devnum, u32 regnum, u32 *val);
extern int rtl931x_write_mmd_phy(u32 port, u32 devnum, u32 regnum, u32 val);

/* On the RTL93XX, the RTL93XX_DMA_IF_RX_RING_CNTR track the fill level of
 * the rings. Writing x into these registers substracts x from its content.
 * When the content reaches the ring size, the ASIC no longer adds
 * packets to this receive queue.
 */
void rtl838x_update_cntr(int r, int released)
{
	/* This feature is not available on RTL838x SoCs */
}

void rtl839x_update_cntr(int r, int released)
{
	/* This feature is not available on RTL839x SoCs */
}

void rtl930x_update_cntr(int r, int released)
{
        u32 v = RTL930X_DMA_IF_RX_RING_CNTR_GET(sw_r32(RTL930X_DMA_IF_RX_RING_CNTR_REG(r)), r);

	sw_w32_mask(RTL930X_DMA_IF_RX_RING_CNTR_SET(r, _RTL930X_DMA_IF_RX_RING_CNTR_MASK),
	            RTL930X_DMA_IF_RX_RING_CNTR_SET(r, released),
	            RTL930X_DMA_IF_RX_RING_CNTR_REG(r));
	sw_w32(v, RTL930X_DMA_IF_RX_RING_CNTR_REG(r));
}

void rtl931x_update_cntr(int r, int released)
{
        u32 v = RTL931X_DMA_IF_RX_RING_CNTR_GET(sw_r32(RTL931X_DMA_IF_RX_RING_CNTR_REG(r)), r);

	sw_w32_mask(RTL931X_DMA_IF_RX_RING_CNTR_SET(r, _RTL931X_DMA_IF_RX_RING_CNTR_MASK),
	            RTL931X_DMA_IF_RX_RING_CNTR_SET(r, released),
	            RTL931X_DMA_IF_RX_RING_CNTR_REG(r));
	sw_w32(v, RTL931X_DMA_IF_RX_RING_CNTR_REG(r));
}

struct dsa_tag {
	u8	reason;
	u8	queue;
	u16	port;
	u8	l2_offloaded;
	u8	prio;
	bool	crc_error;
};

bool rtl838x_decode_tag(struct p_hdr *h, struct dsa_tag *t)
{
	/* cpu_tag[0] is reserved. Fields are off-by-one */
	t->reason = h->cpu_tag[4] & 0xf;
	t->queue = (h->cpu_tag[1] & 0xe0) >> 5;
	t->port = h->cpu_tag[1] & 0x1f;
	t->crc_error = t->reason == 13;

	pr_debug("Reason: %d\n", t->reason);
	if (t->reason != 6) /* NIC_RX_REASON_SPECIAL_TRAP */
		t->l2_offloaded = 1;
	else
		t->l2_offloaded = 0;

	return t->l2_offloaded;
}

bool rtl839x_decode_tag(struct p_hdr *h, struct dsa_tag *t)
{
	/* cpu_tag[0] is reserved. Fields are off-by-one */
	t->reason = h->cpu_tag[5] & 0x1f;
	t->queue = (h->cpu_tag[4] & 0xe000) >> 13;
	t->port = h->cpu_tag[1] & 0x3f;
	t->crc_error = h->cpu_tag[4] & BIT(6);

	pr_debug("Reason: %d\n", t->reason);
	if ((t->reason >= 7 && t->reason <= 13) || /* NIC_RX_REASON_RMA */
	    (t->reason >= 23 && t->reason <= 25))  /* NIC_RX_REASON_SPECIAL_TRAP */
		t->l2_offloaded = 0;
	else
		t->l2_offloaded = 1;

	return t->l2_offloaded;
}

bool rtl930x_decode_tag(struct p_hdr *h, struct dsa_tag *t)
{
	t->reason = h->cpu_tag[7] & 0x3f;
	t->queue =  (h->cpu_tag[2] >> 11) & 0x1f;
	t->port = (h->cpu_tag[0] >> 8) & 0x1f;
	t->crc_error = h->cpu_tag[1] & BIT(6);

	pr_debug("Reason %d, port %d, queue %d\n", t->reason, t->port, t->queue);
	if (t->reason >= 19 && t->reason <= 27)
		t->l2_offloaded = 0;
	else
		t->l2_offloaded = 1;

	return t->l2_offloaded;
}

bool rtl931x_decode_tag(struct p_hdr *h, struct dsa_tag *t)
{
	t->reason = h->cpu_tag[7] & 0x3f;
	t->queue =  (h->cpu_tag[2] >> 11) & 0x1f;
	t->port = (h->cpu_tag[0] >> 8) & 0x3f;
	t->crc_error = h->cpu_tag[1] & BIT(6);

	if (t->reason != 63)
		pr_info("%s: Reason %d, port %d, queue %d\n", __func__, t->reason, t->port, t->queue);
	if (t->reason >= 19 && t->reason <= 27)	/* NIC_RX_REASON_RMA */
		t->l2_offloaded = 0;
	else
		t->l2_offloaded = 1;

	return t->l2_offloaded;
}

/* Discard the RX ring-buffers, called as part of the net-ISR
 * when the buffer runs over
 */
static void rtl838x_rb_cleanup(struct rtl838x_eth_priv *priv, int status)
{
	for (int r = 0; r < priv->rxrings; r++) {
		struct ring_b *ring = priv->membase;
		struct p_hdr *h;
		u32 *last;

		pr_debug("In %s working on r: %d\n", __func__, r);
		last = (u32 *)KSEG1ADDR(sw_r32(priv->r->dma_if_rx_cur + r * 4));
		do {
			if ((ring->rx_r[r][ring->c_rx[r]] & 0x1))
				break;
			pr_debug("Got something: %d\n", ring->c_rx[r]);
			h = &ring->rx_header[r][ring->c_rx[r]];
			memset(h, 0, sizeof(struct p_hdr));
			h->buf = (u8 *)KSEG1ADDR(ring->rx_space +
			                         r * priv->rxringlen * RING_BUFFER +
			                         ring->c_rx[r] * RING_BUFFER);
			h->size = RING_BUFFER;
			/* make sure the header is visible to the ASIC */
			mb();

			ring->rx_r[r][ring->c_rx[r]] = KSEG1ADDR(h) | 0x1 | (ring->c_rx[r] == (priv->rxringlen - 1) ?
			                               WRAP :
			                               0x1);
			ring->c_rx[r] = (ring->c_rx[r] + 1) % priv->rxringlen;
		} while (&ring->rx_r[r][ring->c_rx[r]] != last);
	}
}

struct fdb_update_work {
	struct work_struct work;
	struct net_device *ndev;
	u64 macs[NOTIFY_EVENTS + 1];
};

void rtl838x_fdb_sync(struct work_struct *work)
{
	const struct fdb_update_work *uw = container_of(work, struct fdb_update_work, work);

	for (int i = 0; uw->macs[i]; i++) {
	        struct switchdev_notifier_fdb_info info;
	        u8 addr[ETH_ALEN];
	        int action;

		action = (uw->macs[i] & (1ULL << 63)) ?
		         SWITCHDEV_FDB_ADD_TO_BRIDGE :
		         SWITCHDEV_FDB_DEL_TO_BRIDGE;
		u64_to_ether_addr(uw->macs[i] & 0xffffffffffffULL, addr);
		info.addr = &addr[0];
		info.vid = 0;
		info.offloaded = 1;
		pr_debug("FDB entry %d: %llx, action %d\n", i, uw->macs[0], action);
		call_switchdev_notifiers(action, uw->ndev, &info.info, NULL);
	}
	kfree(work);
}

static void rtl839x_l2_notification_handler(struct rtl838x_eth_priv *priv)
{
	struct notify_b *nb = priv->membase + sizeof(struct ring_b);
        u32 e = priv->lastEvent;

	while (!(nb->ring[e] & 1)) {
                struct fdb_update_work *w;
                struct n_event *event;
                u64 mac;
                int i;

		w = kzalloc(sizeof(*w), GFP_ATOMIC);
		if (!w) {
			pr_err("Out of memory: %s", __func__);
			return;
		}
		INIT_WORK(&w->work, rtl838x_fdb_sync);

		for (i = 0; i < NOTIFY_EVENTS; i++) {
			event = &nb->blocks[e].events[i];
			if (!event->valid)
				continue;
			mac = event->mac;
			if (event->type)
				mac |= 1ULL << 63;
			w->ndev = priv->netdev;
			w->macs[i] = mac;
		}

		/* Hand the ring entry back to the switch */
		nb->ring[e] = nb->ring[e] | 1;
		e = (e + 1) % NOTIFY_BLOCKS;

		w->macs[i] = 0ULL;
		schedule_work(&w->work);
	}
	priv->lastEvent = e;
}

static irqreturn_t rtl83xx_net_irq(int irq, void *dev_id)
{
	struct net_device *dev = dev_id;
	struct rtl838x_eth_priv *priv = netdev_priv(dev);
	u32 status = sw_r32(priv->r->dma_if_intr_sts);

	pr_debug("IRQ: %08x\n", status);

	/*  Ignore TX interrupt */
	if (status & (RTL838X_DMA_IF_INTR_STS_TX_ALL_DONE | RTL838X_DMA_IF_INTR_STS_TX_DONE)) {
		sw_w32((RTL838X_DMA_IF_INTR_STS_TX_ALL_DONE | RTL838X_DMA_IF_INTR_STS_TX_DONE), priv->r->dma_if_intr_sts);
	}

	/* RX interrupt */
	if (status & RTL838X_DMA_IF_INTR_STS_RX_DONE) {
		/* ACK and disable RX interrupt for this ring */
		sw_w32_mask(RTL838X_DMA_IF_INTR_MSK_RX_DONE & status, 0, priv->r->dma_if_intr_msk);
		sw_w32(RTL838X_DMA_IF_INTR_STS_RX_DONE & status, priv->r->dma_if_intr_sts);
		for (int i = 0; i < priv->rxrings; i++) {
			if (status & FIELD_PREP(RTL838X_DMA_IF_INTR_MSK_RX_DONE, DMA_RING(i))) {
				pr_debug("Scheduling queue: %d\n", i);
				napi_schedule(&priv->rx_qs[i].napi);
			}
		}
	}

	/* RX buffer overrun */
	if (status & RTL838X_DMA_IF_INTR_MSK_RUNOUT) {
		pr_debug("RX buffer overrun: status %x, mask: %x\n",
			 status, sw_r32(priv->r->dma_if_intr_msk));
		sw_w32(status, priv->r->dma_if_intr_sts);
		rtl838x_rb_cleanup(priv, status & 0xff);
	}

	if (priv->family_id == RTL8390_FAMILY_ID && (status & RTL839X_DMA_IF_INTR_STS_LOCAL_NTFY_BUF_RUNOUT)) {
		sw_w32(RTL839X_DMA_IF_INTR_MSK_LOCAL_NTFY_BUF_RUNOUT, priv->r->dma_if_intr_sts);
		rtl839x_l2_notification_handler(priv);
	}

	if (priv->family_id == RTL8390_FAMILY_ID && (status & RTL839X_DMA_IF_INTR_STS_NTFY_BF_RUNOUT)) {
		sw_w32(RTL839X_DMA_IF_INTR_STS_NTFY_BF_RUNOUT, priv->r->dma_if_intr_sts);
		rtl839x_l2_notification_handler(priv);
	}

	if (priv->family_id == RTL8390_FAMILY_ID && (status & RTL839X_DMA_IF_INTR_STS_NTFY_DONE)) {
		sw_w32(RTL839X_DMA_IF_INTR_STS_NTFY_DONE, priv->r->dma_if_intr_sts);
		rtl839x_l2_notification_handler(priv);
	}

	return IRQ_HANDLED;
}

static irqreturn_t rtl93xx_net_irq(int irq, void *dev_id)
{
	struct net_device *dev = dev_id;
	struct rtl838x_eth_priv *priv = netdev_priv(dev);
	u32 status_rx_r = sw_r32(priv->r->dma_if_intr_rx_runout_sts);
	u32 status_rx = sw_r32(priv->r->dma_if_intr_rx_done_sts);
	u32 status_tx = sw_r32(priv->r->dma_if_intr_tx_done_sts);

	pr_debug("In %s, status_tx: %08x, status_rx: %08x, status_rx_r: %08x\n",
		__func__, status_tx, status_rx, status_rx_r);

	/*  Ignore TX interrupt */
	if (status_tx) {
		/* Clear ISR */
		pr_debug("TX done\n");
		sw_w32(status_tx, priv->r->dma_if_intr_tx_done_sts);
	}

	/* RX interrupt */
	if (status_rx) {
		pr_debug("RX IRQ\n");
		/* ACK and disable RX interrupt for given rings */
		sw_w32(status_rx, priv->r->dma_if_intr_rx_done_sts);
		sw_w32_mask(status_rx, 0, priv->r->dma_if_intr_rx_done_msk);
		for (int i = 0; i < priv->rxrings; i++) {
			if (status_rx & BIT(i)) {
				pr_debug("Scheduling queue: %d\n", i);
				napi_schedule(&priv->rx_qs[i].napi);
			}
		}
	}

	/* RX buffer overrun */
	if (status_rx_r) {
		pr_debug("RX buffer overrun: status %x, mask: %x\n",
		         status_rx_r, sw_r32(priv->r->dma_if_intr_rx_runout_msk));
		sw_w32(status_rx_r, priv->r->dma_if_intr_rx_runout_sts);
		rtl838x_rb_cleanup(priv, status_rx_r);
	}

	return IRQ_HANDLED;
}

static const struct rtl838x_eth_reg rtl838x_reg = {
	.net_irq = rtl83xx_net_irq,
	.mac_port_ctrl = rtl838x_mac_port_ctrl_eth,
	.dma_if_intr_sts = RTL838X_DMA_IF_INTR_STS_REG,
	.dma_if_intr_msk = RTL838X_DMA_IF_INTR_MSK_REG,
	.dma_if_ctrl = RTL838X_DMA_IF_CTRL_REG,
	.mac_force_mode_ctrl = RTL838X_MAC_FORCE_MODE_CTRL_ETH,
	.dma_rx_base = RTL838X_DMA_IF_RX_BASE_DESC_ADDR_CTRL_REG(0),
	.dma_tx_base = RTL838X_DMA_IF_TX_BASE_DESC_ADDR_CTRL_REG(0),
	.dma_if_rx_ring_size = rtl838x_dma_if_rx_ring_size,
	.dma_if_rx_ring_cntr = rtl838x_dma_if_rx_ring_cntr,
	.dma_if_rx_cur = RTL838X_DMA_IF_RX_CUR,
	.rst_glb_ctrl = RTL838X_RST_GLB_CTRL_0,
	.get_mac_link_sts = rtl838x_get_mac_link_sts,
	.get_mac_link_dup_sts = rtl838x_get_mac_link_dup_sts,
	.get_mac_link_spd_sts = rtl838x_get_mac_link_spd_sts,
	.get_mac_rx_pause_sts = rtl838x_get_mac_rx_pause_sts,
	.get_mac_tx_pause_sts = rtl838x_get_mac_tx_pause_sts,
	.mac = RTL838X_MAC_ADDR_CTRL_HI_REG,
	.l2_tbl_flush_ctrl = RTL838X_L2_TBL_FLUSH_CTRL,
	.update_cntr = rtl838x_update_cntr,
	.create_tx_header = rtl838x_create_tx_header,
	.decode_tag = rtl838x_decode_tag,
};

static const struct rtl838x_eth_reg rtl839x_reg = {
	.net_irq = rtl83xx_net_irq,
	.mac_port_ctrl = rtl839x_mac_port_ctrl_eth,
	.dma_if_intr_sts = RTL839X_DMA_IF_INTR_STS_REG,
	.dma_if_intr_msk = RTL839X_DMA_IF_INTR_MSK_REG,
	.dma_if_ctrl = RTL839X_DMA_IF_CTRL_REG,
	.mac_force_mode_ctrl = RTL839X_MAC_FORCE_MODE_CTRL_ETH,
	.dma_rx_base = RTL839X_DMA_IF_RX_BASE_DESC_ADDR_CTRL_REG(0),
	.dma_tx_base = RTL839X_DMA_IF_TX_BASE_DESC_ADDR_CTRL_REG(0),
	.dma_if_rx_ring_size = rtl839x_dma_if_rx_ring_size,
	.dma_if_rx_ring_cntr = rtl839x_dma_if_rx_ring_cntr,
	.dma_if_rx_cur = RTL839X_DMA_IF_RX_CUR,
	.rst_glb_ctrl = RTL839X_RST_GLB_CTRL,
	.get_mac_link_sts = rtl839x_get_mac_link_sts,
	.get_mac_link_dup_sts = rtl839x_get_mac_link_dup_sts,
	.get_mac_link_spd_sts = rtl839x_get_mac_link_spd_sts,
	.get_mac_rx_pause_sts = rtl839x_get_mac_rx_pause_sts,
	.get_mac_tx_pause_sts = rtl839x_get_mac_tx_pause_sts,
	.mac = RTL839X_MAC_ADDR_CTRL_HI_REG,
	.l2_tbl_flush_ctrl = RTL839X_L2_TBL_FLUSH_CTRL,
	.update_cntr = rtl839x_update_cntr,
	.create_tx_header = rtl839x_create_tx_header,
	.decode_tag = rtl839x_decode_tag,
};

static const struct rtl838x_eth_reg rtl930x_reg = {
	.net_irq = rtl93xx_net_irq,
	.mac_port_ctrl = rtl930x_mac_port_ctrl_eth,
	.dma_if_intr_rx_runout_sts = RTL930X_DMA_IF_INTR_RX_RUNOUT_STS_REG,
	.dma_if_intr_rx_done_sts = RTL930X_DMA_IF_INTR_RX_DONE_STS_REG,
	.dma_if_intr_tx_done_sts = RTL930X_DMA_IF_INTR_TX_DONE_STS_REG,
	.dma_if_intr_rx_runout_msk = RTL930X_DMA_IF_INTR_RX_RUNOUT_MSK_REG,
	.dma_if_intr_rx_done_msk = RTL930X_DMA_IF_INTR_RX_DONE_MSK_REG,
	.dma_if_intr_tx_done_msk = RTL930X_DMA_IF_INTR_TX_DONE_MSK_REG,
	.l2_ntfy_if_intr_sts = RTL930X_L2_NTFY_IF_INTR_STS,
	.l2_ntfy_if_intr_msk = RTL930X_L2_NTFY_IF_INTR_MSK,
	.dma_if_ctrl = RTL930X_DMA_IF_CTRL_REG,
	.mac_force_mode_ctrl = RTL930X_MAC_FORCE_MODE_CTRL_ETH,
	.dma_rx_base = RTL930X_DMA_IF_RX_BASE_DESC_ADDR_CTRL_REG(0),
	.dma_tx_base = RTL930X_DMA_IF_TX_BASE_DESC_ADDR_CTRL_REG(0),
	.dma_if_rx_ring_size = rtl930x_dma_if_rx_ring_size,
	.dma_if_rx_ring_cntr = rtl930x_dma_if_rx_ring_cntr,
	.dma_if_rx_cur = RTL930X_DMA_IF_RX_CUR,
	.rst_glb_ctrl = RTL930X_RST_GLB_CTRL_0,
	.get_mac_link_sts = rtl930x_get_mac_link_sts,
	.get_mac_link_dup_sts = rtl930x_get_mac_link_dup_sts,
	.get_mac_link_spd_sts = rtl930x_get_mac_link_spd_sts,
	.get_mac_rx_pause_sts = rtl930x_get_mac_rx_pause_sts,
	.get_mac_tx_pause_sts = rtl930x_get_mac_tx_pause_sts,
	.mac = RTL930X_MAC_L2_ADDR_CTRL_HI_REG,
	.l2_tbl_flush_ctrl = RTL930X_L2_TBL_FLUSH_CTRL,
	.update_cntr = rtl930x_update_cntr,
	.create_tx_header = rtl930x_create_tx_header,
	.decode_tag = rtl930x_decode_tag,
};

static const struct rtl838x_eth_reg rtl931x_reg = {
	.net_irq = rtl93xx_net_irq,
	.mac_port_ctrl = rtl931x_mac_port_ctrl_eth,
	.dma_if_intr_rx_runout_sts = RTL931X_DMA_IF_INTR_RX_RUNOUT_STS_REG,
	.dma_if_intr_rx_done_sts = RTL931X_DMA_IF_INTR_RX_DONE_STS_REG,
	.dma_if_intr_tx_done_sts = RTL931X_DMA_IF_INTR_TX_DONE_STS_REG,
	.dma_if_intr_rx_runout_msk = RTL931X_DMA_IF_INTR_RX_RUNOUT_MSK_REG,
	.dma_if_intr_rx_done_msk = RTL931X_DMA_IF_INTR_RX_DONE_MSK_REG,
	.dma_if_intr_tx_done_msk = RTL931X_DMA_IF_INTR_TX_DONE_MSK_REG,
	.l2_ntfy_if_intr_sts = RTL931X_L2_NTFY_IF_INTR_STS,
	.l2_ntfy_if_intr_msk = RTL931X_L2_NTFY_IF_INTR_MSK,
	.dma_if_ctrl = RTL931X_DMA_IF_CTRL_REG,
	.mac_force_mode_ctrl = RTL931X_MAC_FORCE_MODE_CTRL_ETH,
	.dma_rx_base = RTL931X_DMA_IF_RX_BASE_DESC_ADDR_CTRL_REG(0),
	.dma_tx_base = RTL931X_DMA_IF_TX_BASE_DESC_ADDR_CTRL_REG(0),
	.dma_if_rx_ring_size = rtl931x_dma_if_rx_ring_size,
	.dma_if_rx_ring_cntr = rtl931x_dma_if_rx_ring_cntr,
	.dma_if_rx_cur = RTL931X_DMA_IF_RX_CUR,
	.rst_glb_ctrl = RTL931X_RST_GLB_CTRL,
	.get_mac_link_sts = rtl931x_get_mac_link_sts,
	.get_mac_link_dup_sts = rtl931x_get_mac_link_dup_sts,
	.get_mac_link_spd_sts = rtl931x_get_mac_link_spd_sts,
	.get_mac_rx_pause_sts = rtl931x_get_mac_rx_pause_sts,
	.get_mac_tx_pause_sts = rtl931x_get_mac_tx_pause_sts,
	.mac = RTL931X_MAC_L2_ADDR_CTRL_HI_REG,
	.l2_tbl_flush_ctrl = RTL931X_L2_TBL_FLUSH_CTRL,
	.update_cntr = rtl931x_update_cntr,
	.create_tx_header = rtl931x_create_tx_header,
	.decode_tag = rtl931x_decode_tag,
};

static void rtl838x_hw_reset(struct rtl838x_eth_priv *priv)
{
	u32 int_saved, nbuf;
	u32 reset_mask;

	pr_info("RESETTING %x, CPU_PORT %d\n", priv->family_id, priv->cpu_port);
	sw_w32_mask(0x3, 0, priv->r->mac_port_ctrl(priv->cpu_port));
	msleep(100);

	switch(priv->family_id) {
	case RTL8380_FAMILY_ID:
		sw_w32(0, RTL838X_DMA_IF_INTR_MSK_REG);
		sw_w32(RTL838X_DMA_IF_INTR_STS_TX_ALL_DONE |
		       RTL838X_DMA_IF_INTR_STS_TX_DONE |
		       RTL838X_DMA_IF_INTR_STS_RX_DONE |
		       RTL838X_DMA_IF_INTR_STS_RUNOUT,
		       RTL838X_DMA_IF_INTR_STS_REG);
		break;
	case RTL8390_FAMILY_ID:
		sw_w32(0, RTL839X_DMA_IF_INTR_MSK_REG);
		sw_w32(RTL839X_DMA_IF_INTR_MSK_NTFY_DONE |
		       RTL839X_DMA_IF_INTR_MSK_NTFY_BF_RUNOUT |
		       RTL839X_DMA_IF_INTR_MSK_LOCAL_NTFY_BUF_RUNOUT |
		       RTL839X_DMA_IF_INTR_MSK_TX_ALL_DONE |
		       RTL839X_DMA_IF_INTR_MSK_TX_DONE |
		       RTL839X_DMA_IF_INTR_MSK_RX_DONE |
		       RTL839X_DMA_IF_INTR_MSK_RX_RUNOUT,
		       RTL839X_DMA_IF_INTR_STS_REG);
		break;
	case RTL9300_FAMILY_ID:
		sw_w32(0, RTL930X_DMA_IF_INTR_RX_RUNOUT_MSK_REG);
		sw_w32(RTL930X_DMA_IF_INTR_RX_RUNOUT_STS_DONE, RTL930X_DMA_IF_INTR_RX_RUNOUT_STS_REG);
		sw_w32(0, RTL930X_DMA_IF_INTR_RX_DONE_MSK_REG);
		sw_w32(RTL930X_DMA_IF_INTR_RX_DONE_STS_DONE, RTL930X_DMA_IF_INTR_RX_DONE_STS_REG);
		sw_w32(0, RTL930X_DMA_IF_INTR_TX_DONE_MSK_REG);
		sw_w32(RTL930X_DMA_IF_INTR_TX_DONE_STS_ALL_DONE |
		       RTL930X_DMA_IF_INTR_TX_DONE_STS_DONE,
		       RTL930X_DMA_IF_INTR_TX_DONE_STS_REG);
		break;
	case RTL9310_FAMILY_ID:
		sw_w32(0, RTL931X_DMA_IF_INTR_RX_RUNOUT_MSK_REG);
		sw_w32(RTL931X_DMA_IF_INTR_RX_RUNOUT_STS_DONE, RTL931X_DMA_IF_INTR_RX_RUNOUT_STS_REG);
		sw_w32(0, RTL931X_DMA_IF_INTR_RX_DONE_MSK_REG);
		sw_w32(RTL931X_DMA_IF_INTR_RX_DONE_STS_DONE, RTL931X_DMA_IF_INTR_RX_DONE_STS_REG);
		sw_w32(0, RTL931X_DMA_IF_INTR_TX_DONE_MSK_REG);
		sw_w32(RTL931X_DMA_IF_INTR_TX_DONE_STS_ALL_DONE |
		       RTL931X_DMA_IF_INTR_TX_DONE_STS_DONE,
		       RTL931X_DMA_IF_INTR_TX_DONE_STS_REG);
		break;
	default:
		pr_err("%s: Unsupported chip family: %d\n", __func__, priv->family_id);
	}

	if (priv->family_id == RTL8390_FAMILY_ID) {
		/* Preserve L2 notification and NBUF settings */
		int_saved = sw_r32(priv->r->dma_if_intr_msk);
		nbuf = sw_r32(RTL839X_DMA_IF_NBUF_BASE_DESC_ADDR_CTRL);

		/* Disable link change interrupt on RTL839x */
		rtl839x_imr_port_link_sts_chg(0x0);

		sw_w32(0, RTL839X_DMA_IF_INTR_MSK_REG);
		sw_w32(RTL839X_DMA_IF_INTR_MSK_NTFY_DONE |
		       RTL839X_DMA_IF_INTR_MSK_NTFY_BF_RUNOUT |
		       RTL839X_DMA_IF_INTR_MSK_LOCAL_NTFY_BUF_RUNOUT |
		       RTL839X_DMA_IF_INTR_MSK_TX_ALL_DONE |
		       RTL839X_DMA_IF_INTR_MSK_TX_DONE |
		       RTL839X_DMA_IF_INTR_MSK_RX_DONE |
		       RTL839X_DMA_IF_INTR_MSK_RX_RUNOUT,
		       RTL839X_DMA_IF_INTR_STS_REG);
	}

	switch (priv->family_id) {
	case RTL8380_FAMILY_ID:
		reset_mask = RTL838X_RST_GLB_CTRL_0_SW_NIC_RST | RTL838X_RST_GLB_CTRL_0_SW_Q_RST;
		break;
	case RTL8390_FAMILY_ID:
		reset_mask = RTL839X_RST_GLB_CTRL_SW_NIC_RST | RTL839X_RST_GLB_CTRL_SW_Q_RST;
		break;
	case RTL9300_FAMILY_ID:
		reset_mask = RTL930X_RST_GLB_CTRL_0_SW_NIC_RST | RTL930X_RST_GLB_CTRL_0_SW_Q_RST;
		break;
	case RTL9310_FAMILY_ID:
		reset_mask = RTL931X_RST_GLB_CTRL_SW_NIC_RST | RTL931X_RST_GLB_CTRL_SW_Q_RST;
		break;
	default:
		pr_err("%s: Unsupported family id: %d", __func__, priv->family_id);
		break;
	}
	sw_w32_mask(0, reset_mask, priv->r->rst_glb_ctrl);

	do { /* Wait for reset of NIC and Queues done */
		udelay(20);
	} while (sw_r32(priv->r->rst_glb_ctrl) & reset_mask);
	msleep(100);

	/* Setup Head of Line */
	if (priv->family_id == RTL8380_FAMILY_ID)
		sw_w32(0, RTL838X_DMA_IF_RX_RING_SIZE_REG(0));  /* Disabled on RTL8380 */
	if (priv->family_id == RTL8390_FAMILY_ID)
		sw_w32(GENMASK(31, 0), RTL839X_DMA_IF_RX_RING_CNTR_REG(0));
	if (priv->family_id == RTL9300_FAMILY_ID || priv->family_id == RTL9310_FAMILY_ID) {
		for (int i = 0; i < priv->rxrings; i++) {
			sw_w32_mask(RTL930X_DMA_IF_RX_RING_SIZE_SET(i, _RTL930X_DMA_IF_RX_RING_SIZE_MASK),
			            FIELD_PREP(_RTL930X_DMA_IF_RX_RING_SIZE_MASK, 0),
			            priv->r->dma_if_rx_ring_size(i));
			sw_w32_mask(RTL930X_DMA_IF_RX_RING_CNTR_SET(i, _RTL930X_DMA_IF_RX_RING_CNTR_MASK),
			            FIELD_PREP(_RTL930X_DMA_IF_RX_RING_CNTR_MASK, priv->rxringlen),
			            priv->r->dma_if_rx_ring_cntr(i));
		}
	}

	/* Re-enable link change interrupt */
	if (priv->family_id == RTL8390_FAMILY_ID) {
		rtl839x_isr_port_link_sts_chg(GENMASK_ULL(RTL839X_PORT_CNT - 1, 0));
		rtl839x_imr_port_link_sts_chg(GENMASK_ULL(RTL839X_PORT_CNT - 1, 0));

		sw_w32_mask(0,
		            RTL839X_DMA_IF_INTR_MSK_NTFY_DONE |
		            RTL839X_DMA_IF_INTR_MSK_NTFY_BF_RUNOUT |
		            RTL839X_DMA_IF_INTR_MSK_LOCAL_NTFY_BUF_RUNOUT |
		            (int_saved & (RTL839X_DMA_IF_INTR_MSK_NTFY_DONE |
		                         RTL839X_DMA_IF_INTR_MSK_NTFY_BF_RUNOUT |
		                         RTL839X_DMA_IF_INTR_MSK_LOCAL_NTFY_BUF_RUNOUT)),
		            priv->r->dma_if_intr_msk);
		sw_w32(nbuf, RTL839X_DMA_IF_NBUF_BASE_DESC_ADDR_CTRL);
	}
}

static void rtl838x_hw_ring_setup(struct rtl838x_eth_priv *priv)
{
	struct ring_b *ring = priv->membase;

	for (int i = 0; i < priv->rxrings; i++)
		sw_w32(KSEG1ADDR(&ring->rx_r[i]), priv->r->dma_rx_base + i * 4);

	for (int i = 0; i < RTL838X_DMA_IF_TX_RING_MAX; i++)
		sw_w32(KSEG1ADDR(&ring->tx_r[i]), priv->r->dma_tx_base + i * 4);
}

static void rtl838x_hw_en_rxtx(struct rtl838x_eth_priv *priv)
{
	/* Disable Head of Line features for all RX rings */
	sw_w32(0xffffffff, RTL838X_DMA_IF_RX_RING_SIZE_REG(0));

	sw_w32(FIELD_PREP(RTL838X_DMA_IF_CTRL_RX_TRUNCATE_LEN, RING_BUFFER) |
	       RTL838X_DMA_IF_CTRL_TX_PAD_EN,
	       RTL838X_DMA_IF_CTRL_REG);

	sw_w32(RTL838X_DMA_IF_INTR_MSK_TX_ALL_DONE |
	       RTL838X_DMA_IF_INTR_MSK_TX_DONE |
	       RTL838X_DMA_IF_INTR_MSK_RX_DONE |
	       RTL838X_DMA_IF_INTR_MSK_RUNOUT,
	       RTL838X_DMA_IF_INTR_MSK_REG);

	sw_w32_mask(0,
	            RTL838X_DMA_IF_CTRL_RX_EN |
	            RTL838X_DMA_IF_CTRL_TX_EN,
	            RTL838X_DMA_IF_CTRL_REG);

	/* Restart TX/RX to CPU port */
	sw_w32_mask(0x0, 0x3, priv->r->mac_port_ctrl(priv->cpu_port));
	/* Set Speed, duplex, flow control
	 * FORCE_EN | LINK_EN | NWAY_EN | DUP_SEL
	 * | SPD_SEL = 0b10 | FORCE_FC_EN | PHY_MASTER_SLV_MANUAL_EN
	 * | MEDIA_SEL
	 */
	sw_w32(0x6192F, priv->r->mac_force_mode_ctrl + priv->cpu_port * 4);

	/* Enable CRC checks on CPU-port */
	sw_w32_mask(0, BIT(3), priv->r->mac_port_ctrl(priv->cpu_port));
}

static void rtl839x_hw_en_rxtx(struct rtl838x_eth_priv *priv)
{
	sw_w32(FIELD_PREP(RTL839X_DMA_IF_CTRL_RX_TRUNCATE_LEN, RING_BUFFER) |
	       RTL839X_DMA_IF_CTRL_TX_EN,
	       RTL839X_DMA_IF_CTRL_REG);

	sw_w32(RTL839X_DMA_IF_INTR_MSK_NTFY_DONE |
	       RTL839X_DMA_IF_INTR_MSK_NTFY_BF_RUNOUT |
	       RTL839X_DMA_IF_INTR_MSK_LOCAL_NTFY_BUF_RUNOUT |
	       RTL839X_DMA_IF_INTR_MSK_TX_ALL_DONE |
	       RTL839X_DMA_IF_INTR_MSK_TX_DONE |
	       RTL839X_DMA_IF_INTR_MSK_RX_DONE |
	       RTL839X_DMA_IF_INTR_MSK_RX_RUNOUT,
	       RTL839X_DMA_IF_INTR_MSK_REG);

	sw_w32_mask(0,
	            RTL839X_DMA_IF_CTRL_RX_EN |
	            RTL839X_DMA_IF_CTRL_TX_EN,
	            RTL839X_DMA_IF_CTRL_REG);

	/* Restart TX/RX to CPU port, enable CRC checking */
	sw_w32_mask(0x0, 0x3 | BIT(3), priv->r->mac_port_ctrl(priv->cpu_port));

	/* CPU port joins Lookup Miss Flooding Portmask */
	/* TODO: The code below should also work for the RTL838x */
	sw_w32(0x28000, RTL839X_TBL_ACCESS_L2_CTRL);
	sw_w32_mask(0, 0x80000000, RTL839X_TBL_ACCESS_L2_DATA(0));
	sw_w32(0x38000, RTL839X_TBL_ACCESS_L2_CTRL);

	/* Force CPU port link up */
	sw_w32_mask(0, 3, priv->r->mac_force_mode_ctrl + priv->cpu_port * 4);
}

static void rtl93xx_hw_en_rxtx(struct rtl838x_eth_priv *priv)
{
	/* Setup CPU-Port: RX Buffer truncated at 1600 Bytes */
	sw_w32(FIELD_PREP(RTL930X_DMA_IF_CTRL_RX_TRUNCATE_LEN, RING_BUFFER) |
	       RTL930X_DMA_IF_CTRL_RX_TRUNCATE_EN,
	       priv->r->dma_if_ctrl);

	for (int i = 0; i < priv->rxrings; i++) {
		sw_w32_mask(RTL930X_DMA_IF_RX_RING_SIZE_SET(i, _RTL930X_DMA_IF_RX_RING_SIZE_MASK),
		            RTL930X_DMA_IF_RX_RING_SIZE_SET(i, priv->rxringlen),
		            priv->r->dma_if_rx_ring_size(i));

		/* Some SoCs have issues with missing underflow protection */
		sw_w32_mask(RTL930X_DMA_IF_RX_RING_CNTR_SET(i, _RTL930X_DMA_IF_RX_RING_CNTR_MASK),
		            RTL930X_DMA_IF_RX_RING_CNTR_GET(sw_r32(RTL930X_DMA_IF_RX_RING_CNTR_REG(i)), i),
		            priv->r->dma_if_rx_ring_cntr(i));
	}

	sw_w32(RTL930X_DMA_IF_INTR_RX_RUNOUT_MSK_DONE, priv->r->dma_if_intr_rx_runout_msk);
	sw_w32(RTL930X_DMA_IF_INTR_RX_DONE_MSK_DONE, priv->r->dma_if_intr_rx_done_msk);
	sw_w32(RTL930X_DMA_IF_INTR_TX_DONE_MSK_DONE, priv->r->dma_if_intr_tx_done_msk);

	sw_w32_mask(0,
	            RTL930X_DMA_IF_CTRL_RX_EN |
	            RTL930X_DMA_IF_CTRL_TX_EN,
	            priv->r->dma_if_ctrl);

	/* Restart TX/RX to CPU port, enable CRC checking */
	sw_w32_mask(0x0, 0x3 | BIT(4), priv->r->mac_port_ctrl(priv->cpu_port));

	if (priv->family_id == RTL9300_FAMILY_ID)
		sw_w32_mask(0, BIT(priv->cpu_port), RTL930X_L2_UNKN_UC_FLD_PMSK);
	else
		sw_w32_mask(0, BIT(priv->cpu_port), RTL931X_L2_UNKN_UC_FLD_PMSK);

	if (priv->family_id == RTL9300_FAMILY_ID)
		sw_w32(0x217, priv->r->mac_force_mode_ctrl + priv->cpu_port * 4);
	else
		sw_w32(0x2a1d, priv->r->mac_force_mode_ctrl + priv->cpu_port * 4);
}

static void rtl838x_setup_ring_buffer(struct rtl838x_eth_priv *priv, struct ring_b *ring)
{
	for (int i = 0; i < priv->rxrings; i++) {
	        struct p_hdr *h;
                int j;

		for (j = 0; j < priv->rxringlen; j++) {
			h = &ring->rx_header[i][j];
			memset(h, 0, sizeof(struct p_hdr));
			h->buf = (u8 *)KSEG1ADDR(ring->rx_space +
			                         i * priv->rxringlen * RING_BUFFER +
			                         j * RING_BUFFER);
			h->size = RING_BUFFER;
			/* All rings owned by switch, last one wraps */
			ring->rx_r[i][j] = KSEG1ADDR(h) | 1 | (j == (priv->rxringlen - 1) ?
			                   WRAP :
			                   0);
		}
		ring->c_rx[i] = 0;
	}

	for (int i = 0; i < RTL838X_DMA_IF_TX_RING_MAX; i++) {
		struct p_hdr *h;
		int j;

		for (j = 0; j < RTL838X_DMA_IF_TX_RING_LEN; j++) {
			h = &ring->tx_header[i][j];
			memset(h, 0, sizeof(struct p_hdr));
			h->buf = (u8 *)KSEG1ADDR(ring->tx_space +
			                         i * RTL838X_DMA_IF_TX_RING_LEN * RING_BUFFER +
			                         j * RING_BUFFER);
			h->size = RING_BUFFER;
			ring->tx_r[i][j] = KSEG1ADDR(&ring->tx_header[i][j]);
		}
		/* Last header is wrapping around */
		ring->tx_r[i][j - 1] |= WRAP;
		ring->c_tx[i] = 0;
	}
}

static void rtl839x_setup_notify_ring_buffer(struct rtl838x_eth_priv *priv)
{
	struct notify_b *b = priv->membase + sizeof(struct ring_b);

	for (int i = 0; i < NOTIFY_BLOCKS; i++)
		b->ring[i] = KSEG1ADDR(&b->blocks[i]) | 1 | (i == (NOTIFY_BLOCKS - 1) ? WRAP : 0);

	sw_w32((u32) b->ring, RTL839X_DMA_IF_NBUF_BASE_DESC_ADDR_CTRL);
	sw_w32_mask(0x3ff << 2, 100 << 2, RTL839X_L2_NOTIFICATION_CTRL);

	/* Setup notification events */
	sw_w32_mask(0, 1 << 14, RTL839X_L2_CTRL_0); /* RTL8390_L2_CTRL_0_FLUSH_NOTIFY_EN */
	sw_w32_mask(0, 1 << 12, RTL839X_L2_NOTIFICATION_CTRL); /* SUSPEND_NOTIFICATION_EN */

	/* Enable Notification */
	sw_w32_mask(0, 1 << 0, RTL839X_L2_NOTIFICATION_CTRL);
	priv->lastEvent = 0;
}

static int rtl838x_eth_open(struct net_device *ndev)
{
	unsigned long flags;
	struct rtl838x_eth_priv *priv = netdev_priv(ndev);
	struct ring_b *ring = priv->membase;

	pr_debug("%s called: RX rings %d(length %d), TX rings %d(length %d)\n",
		__func__, priv->rxrings, priv->rxringlen, RTL838X_DMA_IF_TX_RING_MAX, RTL838X_DMA_IF_TX_RING_LEN);

	spin_lock_irqsave(&priv->lock, flags);
	rtl838x_hw_reset(priv);
	rtl838x_setup_ring_buffer(priv, ring);
	if (priv->family_id == RTL8390_FAMILY_ID) {
		rtl839x_setup_notify_ring_buffer(priv);
		/* Make sure the ring structure is visible to the ASIC */
		mb();
		flush_cache_all();
	}

	rtl838x_hw_ring_setup(priv);
	phylink_start(priv->phylink);

	for (int i = 0; i < priv->rxrings; i++)
		napi_enable(&priv->rx_qs[i].napi);

	switch (priv->family_id) {
	case RTL8380_FAMILY_ID:
		rtl838x_hw_en_rxtx(priv);
		/* Trap IGMP/MLD traffic to CPU-Port */
		sw_w32(0x3, RTL838X_SPCL_TRAP_IGMP_CTRL);
		/* Flush learned FDB entries on link down of a port */
		sw_w32_mask(0, BIT(7), RTL838X_L2_CTRL_0);
		break;

	case RTL8390_FAMILY_ID:
		rtl839x_hw_en_rxtx(priv);
		/* Trap MLD and IGMP messages to CPU_PORT */
		sw_w32(0x3, RTL839X_SPCL_TRAP_IGMP_CTRL);
		/* Flush learned FDB entries on link down of a port */
		sw_w32_mask(0, BIT(7), RTL839X_L2_CTRL_0);
		break;

	case RTL9300_FAMILY_ID:
		rtl93xx_hw_en_rxtx(priv);
		/* Flush learned FDB entries on link down of a port */
		sw_w32_mask(0, BIT(7), RTL930X_L2_CTRL);
		/* Trap MLD and IGMP messages to CPU_PORT */
		sw_w32((0x2 << 3) | 0x2,  RTL930X_VLAN_APP_PKT_CTRL);
		break;

	case RTL9310_FAMILY_ID:
		rtl93xx_hw_en_rxtx(priv);

		/* Trap MLD and IGMP messages to CPU_PORT */
		sw_w32((0x2 << 3) | 0x2,  RTL931X_VLAN_APP_PKT_CTRL);

		/* Disable External CPU access to switch, clear EXT_CPU_EN */
		sw_w32_mask(BIT(2), 0, RTL931X_MAC_L2_GLOBAL_CTRL2);

		/* Set PCIE_PWR_DOWN */
		sw_w32_mask(0, BIT(1), RTL931X_PS_SOC_CTRL);
		break;
	default:
		pr_err("%s: unsupport chip family: %d\n", __func__, priv->family_id);
		break;
	}

	netif_tx_start_all_queues(ndev);

	spin_unlock_irqrestore(&priv->lock, flags);

	return 0;
}

static void rtl838x_hw_stop(struct rtl838x_eth_priv *priv)
{
	u32 force_mac = priv->family_id == RTL8380_FAMILY_ID ? 0x6192C : 0x75;

	/* Disable RX/TX from/to CPU-port */
	sw_w32_mask(0x3, 0, priv->r->mac_port_ctrl(priv->cpu_port));

	switch(priv->family_id) {
	case RTL8380_FAMILY_ID:
		sw_w32_mask(RTL838X_DMA_IF_CTRL_RX_EN |
		            RTL838X_DMA_IF_CTRL_TX_EN,
		            0,
		            RTL838X_DMA_IF_CTRL_REG);
		break;
	case RTL8390_FAMILY_ID:
		sw_w32_mask(RTL839X_DMA_IF_CTRL_RX_EN |
		            RTL839X_DMA_IF_CTRL_TX_EN,
		            0,
		            RTL839X_DMA_IF_CTRL_REG);
		break;
	case RTL9300_FAMILY_ID:
		sw_w32_mask(RTL930X_DMA_IF_CTRL_RX_EN |
		            RTL930X_DMA_IF_CTRL_TX_EN,
		            0,
		            RTL930X_DMA_IF_CTRL_REG);
		break;
	case RTL9310_FAMILY_ID:
		sw_w32_mask(RTL931X_DMA_IF_CTRL_RX_EN |
		            RTL931X_DMA_IF_CTRL_TX_EN,
		            0,
		            RTL931X_DMA_IF_CTRL_REG);
		break;
	default:
		pr_err("%s: Unsupported chip family: %d\n", __func__, priv->family_id);
		break;
	}
	msleep(200); // Test, whether this is needed

	/* Block all ports */
	if (priv->family_id == RTL8380_FAMILY_ID) {
		sw_w32(0x03000000, RTL838X_TBL_ACCESS_DATA_0(0));
		sw_w32(0x00000000, RTL838X_TBL_ACCESS_DATA_0(1));
		sw_w32(1 << 15 | 2 << 12, RTL838X_TBL_ACCESS_CTRL_0);
	}

	/* Flush L2 address cache */
	if (priv->family_id == RTL8380_FAMILY_ID) {
		for (int i = 0; i <= priv->cpu_port; i++) {
			sw_w32(1 << 26 | 1 << 23 | i << 5, priv->r->l2_tbl_flush_ctrl);
			do { } while (sw_r32(priv->r->l2_tbl_flush_ctrl) & (1 << 26));
		}
	} else if (priv->family_id == RTL8390_FAMILY_ID) {
		for (int i = 0; i <= priv->cpu_port; i++) {
			sw_w32(1 << 28 | 1 << 25 | i << 5, priv->r->l2_tbl_flush_ctrl);
			do { } while (sw_r32(priv->r->l2_tbl_flush_ctrl) & (1 << 28));
		}
	}
	/* TODO: L2 flush register is 64 bit on RTL931X and 930X */

	/* CPU-Port: Link down */
	if (priv->family_id == RTL8380_FAMILY_ID || priv->family_id == RTL8390_FAMILY_ID)
		sw_w32(force_mac, priv->r->mac_force_mode_ctrl + priv->cpu_port * 4);
	else if (priv->family_id == RTL9300_FAMILY_ID)
		sw_w32_mask(0x3, 0, priv->r->mac_force_mode_ctrl + priv->cpu_port *4);
	else if (priv->family_id == RTL9310_FAMILY_ID)
		sw_w32_mask(BIT(0) | BIT(9), 0, priv->r->mac_force_mode_ctrl + priv->cpu_port *4);
	msleep(100);

	switch(priv->family_id) {
	case RTL8380_FAMILY_ID:
		sw_w32(0, RTL838X_DMA_IF_INTR_MSK_REG);
		sw_w32(RTL838X_DMA_IF_INTR_STS_TX_ALL_DONE |
		       RTL838X_DMA_IF_INTR_STS_TX_DONE |
		       RTL838X_DMA_IF_INTR_STS_RX_DONE |
		       RTL838X_DMA_IF_INTR_STS_RUNOUT,
		       RTL838X_DMA_IF_INTR_STS_REG);
		break;
	case RTL8390_FAMILY_ID:
		sw_w32(0, RTL839X_DMA_IF_INTR_MSK_REG);
		sw_w32(RTL839X_DMA_IF_INTR_MSK_NTFY_DONE |
		       RTL839X_DMA_IF_INTR_MSK_NTFY_BF_RUNOUT |
		       RTL839X_DMA_IF_INTR_MSK_LOCAL_NTFY_BUF_RUNOUT |
		       RTL839X_DMA_IF_INTR_MSK_TX_ALL_DONE |
		       RTL839X_DMA_IF_INTR_MSK_TX_DONE |
		       RTL839X_DMA_IF_INTR_MSK_RX_DONE |
		       RTL839X_DMA_IF_INTR_MSK_RX_RUNOUT,
		       RTL839X_DMA_IF_INTR_STS_REG);
		break;
	case RTL9300_FAMILY_ID:
		sw_w32(0, RTL930X_DMA_IF_INTR_RX_RUNOUT_MSK_REG);
		sw_w32(RTL930X_DMA_IF_INTR_RX_RUNOUT_STS_DONE, RTL930X_DMA_IF_INTR_RX_RUNOUT_STS_REG);
		sw_w32(0, RTL930X_DMA_IF_INTR_RX_DONE_MSK_REG);
		sw_w32(RTL930X_DMA_IF_INTR_RX_DONE_STS_DONE, RTL930X_DMA_IF_INTR_RX_DONE_STS_REG);
		sw_w32(0, RTL930X_DMA_IF_INTR_TX_DONE_MSK_REG);
		sw_w32(RTL930X_DMA_IF_INTR_TX_DONE_STS_ALL_DONE |
		       RTL930X_DMA_IF_INTR_TX_DONE_STS_DONE,
		       RTL930X_DMA_IF_INTR_TX_DONE_STS_REG);
		break;
	case RTL9310_FAMILY_ID:
		sw_w32(0, RTL931X_DMA_IF_INTR_RX_RUNOUT_MSK_REG);
		sw_w32(RTL931X_DMA_IF_INTR_RX_RUNOUT_STS_DONE, RTL931X_DMA_IF_INTR_RX_RUNOUT_STS_REG);
		sw_w32(0, RTL931X_DMA_IF_INTR_RX_DONE_MSK_REG);
		sw_w32(RTL931X_DMA_IF_INTR_RX_DONE_STS_DONE, RTL931X_DMA_IF_INTR_RX_DONE_STS_REG);
		sw_w32(0, RTL931X_DMA_IF_INTR_TX_DONE_MSK_REG);
		sw_w32(RTL931X_DMA_IF_INTR_TX_DONE_STS_ALL_DONE |
		       RTL931X_DMA_IF_INTR_TX_DONE_STS_DONE,
		       RTL931X_DMA_IF_INTR_TX_DONE_STS_REG);
		break;
	default:
		pr_warn("%s: Unsupported chip family: %d\n", __func__, priv->family_id);
	}

	/* Disable TX/RX DMA */
	sw_w32(0x00000000, priv->r->dma_if_ctrl);
	msleep(200);
}

static int rtl838x_eth_stop(struct net_device *ndev)
{
	struct rtl838x_eth_priv *priv = netdev_priv(ndev);

	pr_info("in %s\n", __func__);

	phylink_stop(priv->phylink);
	rtl838x_hw_stop(priv);

	for (int i = 0; i < priv->rxrings; i++)
		napi_disable(&priv->rx_qs[i].napi);

	netif_tx_stop_all_queues(ndev);

	return 0;
}

static void rtl838x_eth_set_multicast_list(struct net_device *ndev)
{
	/* Flood all classes of RMA addresses (01-80-C2-00-00-{01..2F})
	 * CTRL_0_FULL = GENMASK(21, 0) = 0x3FFFFF
	 */
	if (!(ndev->flags & (IFF_PROMISC | IFF_ALLMULTI))) {
		sw_w32(0x0, RTL838X_RMA_CTRL_0);
		sw_w32(0x0, RTL838X_RMA_CTRL_1);
	}
	if (ndev->flags & IFF_ALLMULTI)
		sw_w32(GENMASK(21, 0), RTL838X_RMA_CTRL_0);
	if (ndev->flags & IFF_PROMISC) {
		sw_w32(GENMASK(21, 0), RTL838X_RMA_CTRL_0);
		sw_w32(GENMASK(14, 0), RTL838X_RMA_CTRL_1);
	}
}

static void rtl839x_eth_set_multicast_list(struct net_device *ndev)
{
	/* Flood all classes of RMA addresses (01-80-C2-00-00-{01..2F})
	 * CTRL_0_FULL = GENMASK(31, 2) = 0xFFFFFFFC
	 * Lower two bits are reserved, corresponding to RMA 01-80-C2-00-00-00
	 * CTRL_1_FULL = CTRL_2_FULL = GENMASK(31, 0)
	 */
	if (!(ndev->flags & (IFF_PROMISC | IFF_ALLMULTI))) {
		sw_w32(0x0, RTL839X_RMA_CTRL_0);
		sw_w32(0x0, RTL839X_RMA_CTRL_1);
		sw_w32(0x0, RTL839X_RMA_CTRL_2);
		sw_w32(0x0, RTL839X_RMA_CTRL_3);
	}
	if (ndev->flags & IFF_ALLMULTI) {
		sw_w32(GENMASK(31, 2), RTL839X_RMA_CTRL_0);
		sw_w32(GENMASK(31, 0), RTL839X_RMA_CTRL_1);
		sw_w32(GENMASK(31, 0), RTL839X_RMA_CTRL_2);
	}
	if (ndev->flags & IFF_PROMISC) {
		sw_w32(GENMASK(31, 2), RTL839X_RMA_CTRL_0);
		sw_w32(GENMASK(31, 0), RTL839X_RMA_CTRL_1);
		sw_w32(GENMASK(31, 0), RTL839X_RMA_CTRL_2);
		sw_w32(GENMASK(10, 0), RTL839X_RMA_CTRL_3);
	}
}

static void rtl930x_eth_set_multicast_list(struct net_device *ndev)
{
	/* Flood all classes of RMA addresses (01-80-C2-00-00-{01..2F})
	 * CTRL_0_FULL = GENMASK(31, 2) = 0xFFFFFFFC
	 * Lower two bits are reserved, corresponding to RMA 01-80-C2-00-00-00
	 * CTRL_1_FULL = CTRL_2_FULL = GENMASK(31, 0)
	 */
	if (ndev->flags & (IFF_ALLMULTI | IFF_PROMISC)) {
		sw_w32(GENMASK(31, 2), RTL930X_RMA_CTRL_0);
		sw_w32(GENMASK(31, 0), RTL930X_RMA_CTRL_1);
		sw_w32(GENMASK(31, 0), RTL930X_RMA_CTRL_2);
	} else {
		sw_w32(0x0, RTL930X_RMA_CTRL_0);
		sw_w32(0x0, RTL930X_RMA_CTRL_1);
		sw_w32(0x0, RTL930X_RMA_CTRL_2);
	}
}

static void rtl931x_eth_set_multicast_list(struct net_device *ndev)
{
	/* Flood all classes of RMA addresses (01-80-C2-00-00-{01..2F})
	 * CTRL_0_FULL = GENMASK(31, 2) = 0xFFFFFFFC
	 * Lower two bits are reserved, corresponding to RMA 01-80-C2-00-00-00.
	 * CTRL_1_FULL = CTRL_2_FULL = GENMASK(31, 0)
	 */
	if (ndev->flags & (IFF_ALLMULTI | IFF_PROMISC)) {
		sw_w32(GENMASK(31, 2), RTL931X_RMA_CTRL_0);
		sw_w32(GENMASK(31, 0), RTL931X_RMA_CTRL_1);
		sw_w32(GENMASK(31, 0), RTL931X_RMA_CTRL_2);
	} else {
		sw_w32(0x0, RTL931X_RMA_CTRL_0);
		sw_w32(0x0, RTL931X_RMA_CTRL_1);
		sw_w32(0x0, RTL931X_RMA_CTRL_2);
	}
}

static void rtl838x_eth_tx_timeout(struct net_device *ndev, unsigned int txqueue)
{
	unsigned long flags;
	struct rtl838x_eth_priv *priv = netdev_priv(ndev);

	pr_warn("%s\n", __func__);
	spin_lock_irqsave(&priv->lock, flags);
	rtl838x_hw_stop(priv);
	rtl838x_hw_ring_setup(priv);
	switch (priv->family_id) {
	case RTL8380_FAMILY_ID:
		rtl838x_hw_en_rxtx(priv);
		break;
	case RTL8390_FAMILY_ID:
		rtl839x_hw_en_rxtx(priv);
		break;
	case RTL9300_FAMILY_ID:
		rtl93xx_hw_en_rxtx(priv);
		break;
	case RTL9310_FAMILY_ID:
		rtl93xx_hw_en_rxtx(priv);
		break;
	default:
		pr_err("%s: Unsupported chip family: %d\n", __func__, priv->family_id);
		break;
	}
	netif_trans_update(ndev);
	netif_start_queue(ndev);
	spin_unlock_irqrestore(&priv->lock, flags);
}

static int rtl838x_eth_tx(struct sk_buff *skb, struct net_device *dev)
{
	int len;
	struct rtl838x_eth_priv *priv = netdev_priv(dev);
	struct ring_b *ring = priv->membase;
	int ret;
	unsigned long flags;
	struct p_hdr *h;
	int dest_port = -1;
	int q = skb_get_queue_mapping(skb) % RTL838X_DMA_IF_TX_RING_MAX;

	if (q) /* Check for high prio queue */
		pr_debug("SKB priority: %d\n", skb->priority);

	spin_lock_irqsave(&priv->lock, flags);
	len = skb->len;

	/* Check for DSA tagging at the end of the buffer */
	if (netdev_uses_dsa(dev) &&
	    skb->data[len - 4] == 0x80 &&
	    skb->data[len - 3] < priv->cpu_port &&
	    skb->data[len - 2] == 0x10 &&
	    skb->data[len - 1] == 0x00) {
		/* Reuse tag space for CRC if possible */
		dest_port = skb->data[len - 3];
		skb->data[len - 4] = skb->data[len - 3] = skb->data[len - 2] = skb->data[len - 1] = 0x00;
		len -= 4;
	}

	len += 4; /* Add space for CRC */

	if (skb_padto(skb, len)) {
		ret = NETDEV_TX_OK;
		goto txdone;
	}

	/* We can send this packet if CPU owns the descriptor */
	if (!(ring->tx_r[q][ring->c_tx[q]] & 0x1)) {

		/* Set descriptor for tx */
		h = &ring->tx_header[q][ring->c_tx[q]];
		h->size = len;
		h->len = len;
		/* On RTL8380 SoCs, small packet lengths being sent need adjustments */
		if (priv->family_id == RTL8380_FAMILY_ID) {
			if (len < ETH_ZLEN - 4)
				h->len -= 4;
		}

		if (dest_port >= 0)
			priv->r->create_tx_header(h, dest_port, skb->priority >> 1);

		/* Copy packet data to tx buffer */
		memcpy((void *)KSEG1ADDR(h->buf), skb->data, len);
		/* Make sure packet data is visible to ASIC */
		wmb();

		/* Hand over to switch */
		ring->tx_r[q][ring->c_tx[q]] |= 1;

		/* Before starting TX, prevent a Lextra bus bug on RTL8380 SoCs */
		if (priv->family_id == RTL8380_FAMILY_ID) {
			for (int i = 0; i < 10; i++) {
				u32 val = sw_r32(priv->r->dma_if_ctrl);
				if ((val & (RTL838X_DMA_IF_CTRL_TX_EN | RTL838X_DMA_IF_CTRL_RX_EN)) ==
				    (RTL838X_DMA_IF_CTRL_TX_EN | RTL838X_DMA_IF_CTRL_RX_EN))
					break;
			}
		}

		/* Tell switch to send data */
		if (priv->family_id == RTL9310_FAMILY_ID || priv->family_id == RTL9300_FAMILY_ID) {
			/* Ring ID q == 0: Low priority, Ring ID = 1: High prio queue */
			if (!q)
				sw_w32_mask(0, RTL930X_DMA_IF_CTRL_TX_LOW_FETCH, priv->r->dma_if_ctrl);
			else
				sw_w32_mask(0, RTL930X_DMA_IF_CTRL_TX_HIGH_FETCH, priv->r->dma_if_ctrl);
		} else {
			sw_w32_mask(0, RTL838X_DMA_IF_CTRL_TX_FETCH | RTL838X_DMA_IF_CTRL_TX_BUSY, priv->r->dma_if_ctrl);
		}

		dev->stats.tx_packets++;
		dev->stats.tx_bytes += len;
		dev_kfree_skb(skb);
		ring->c_tx[q] = (ring->c_tx[q] + 1) % RTL838X_DMA_IF_TX_RING_LEN;
		ret = NETDEV_TX_OK;
	} else {
		dev_warn(&priv->pdev->dev, "Data is owned by switch\n");
		ret = NETDEV_TX_BUSY;
	}

txdone:
	spin_unlock_irqrestore(&priv->lock, flags);

	return ret;
}

/* Return queue number for TX. On the RTL83XX, these queues have equal priority
 * so we do round-robin
 */
u16 rtl83xx_pick_tx_queue(struct net_device *dev, struct sk_buff *skb,
			  struct net_device *sb_dev)
{
	static u8 last = 0;

	last++;
	return last % RTL838X_DMA_IF_TX_RING_MAX;
}

/* Return queue number for TX. On the RTL93XX, queue 1 is the high priority queue
 */
u16 rtl93xx_pick_tx_queue(struct net_device *dev, struct sk_buff *skb,
			  struct net_device *sb_dev)
{
	if (skb->priority >= TC_PRIO_CONTROL)
		return 1;

	return 0;
}

static int rtl838x_hw_receive(struct net_device *dev, int r, int budget)
{
	struct rtl838x_eth_priv *priv = netdev_priv(dev);
	struct ring_b *ring = priv->membase;
	LIST_HEAD(rx_list);
	unsigned long flags;
	int work_done = 0;
	u32	*last;
	bool dsa = netdev_uses_dsa(dev);

	pr_debug("---------------------------------------------------------- RX - %d\n", r);
	spin_lock_irqsave(&priv->lock, flags);
	last = (u32 *)KSEG1ADDR(sw_r32(priv->r->dma_if_rx_cur + r * 4));

	do {
		struct sk_buff *skb;
		struct dsa_tag tag;
		struct p_hdr *h;
		u8 *skb_data;
		u8 *data;
		int len;

		if ((ring->rx_r[r][ring->c_rx[r]] & 0x1)) {
			if (&ring->rx_r[r][ring->c_rx[r]] != last) {
				netdev_warn(dev, "Ring contention: r: %x, last %x, cur %x\n",
				    r, (uint32_t)last, (u32) &ring->rx_r[r][ring->c_rx[r]]);
			}
			break;
		}

		h = &ring->rx_header[r][ring->c_rx[r]];
		data = (u8 *)KSEG1ADDR(h->buf);
		len = h->len;
		if (!len)
			break;
		work_done++;

		len -= 4; /* strip the CRC */
		/* Add 4 bytes for cpu_tag */
		if (dsa)
			len += 4;

		skb = netdev_alloc_skb(dev, len + 4);
		skb_reserve(skb, NET_IP_ALIGN);

		if (likely(skb)) {
			/* BUG: Prevent bug on RTL838x SoCs */
			if (priv->family_id == RTL8380_FAMILY_ID) {
				sw_w32(GENMASK(31, 0), priv->r->dma_if_rx_ring_size(0));
				for (int i = 0; i < priv->rxrings; i++) {
					unsigned int val;

					/* Update each ring cnt */
					val = sw_r32(priv->r->dma_if_rx_ring_cntr(i));
					sw_w32(val, priv->r->dma_if_rx_ring_cntr(i));
				}
			}

			skb_data = skb_put(skb, len);
			/* Make sure data is visible */
			mb();
			memcpy(skb->data, (u8 *)KSEG1ADDR(data), len);
			/* Overwrite CRC with cpu_tag */
			if (dsa) {
				priv->r->decode_tag(h, &tag);
				skb->data[len - 4] = 0x80;
				skb->data[len - 3] = tag.port;
				skb->data[len - 2] = 0x10;
				skb->data[len - 1] = 0x00;
				if (tag.l2_offloaded)
					skb->data[len - 3] |= 0x40;
			}

			if (tag.queue >= 0)
				pr_debug("Queue: %d, len: %d, reason %d port %d\n",
					 tag.queue, len, tag.reason, tag.port);

			skb->protocol = eth_type_trans(skb, dev);
			if (dev->features & NETIF_F_RXCSUM) {
				if (tag.crc_error)
					skb_checksum_none_assert(skb);
				else
					skb->ip_summed = CHECKSUM_UNNECESSARY;
			}
			dev->stats.rx_packets++;
			dev->stats.rx_bytes += len;

			list_add_tail(&skb->list, &rx_list);
		} else {
			if (net_ratelimit())
				dev_warn(&dev->dev, "low on memory - packet dropped\n");
			dev->stats.rx_dropped++;
		}

		/* Reset header structure */
		memset(h, 0, sizeof(struct p_hdr));
		h->buf = data;
		h->size = RING_BUFFER;

		ring->rx_r[r][ring->c_rx[r]] = KSEG1ADDR(h) | 0x1 | (ring->c_rx[r] == (priv->rxringlen - 1) ?
		                               WRAP :
		                               0x1);
		ring->c_rx[r] = (ring->c_rx[r] + 1) % priv->rxringlen;
		last = (u32 *)KSEG1ADDR(sw_r32(priv->r->dma_if_rx_cur + r * 4));
	} while (&ring->rx_r[r][ring->c_rx[r]] != last && work_done < budget);

	netif_receive_skb_list(&rx_list);

	/* Update counters */
	priv->r->update_cntr(r, 0);

	spin_unlock_irqrestore(&priv->lock, flags);

	return work_done;
}

static int rtl838x_poll_rx(struct napi_struct *napi, int budget)
{
	struct rtl838x_rx_q *rx_q = container_of(napi, struct rtl838x_rx_q, napi);
	struct rtl838x_eth_priv *priv = rx_q->priv;
	int work_done = 0;
	int r = rx_q->id;
	int work;

	while (work_done < budget) {
		work = rtl838x_hw_receive(priv->netdev, r, budget - work_done);
		if (!work)
			break;
		work_done += work;
	}

	if (work_done < budget) {
		napi_complete_done(napi, work_done);

		switch(priv->family_id) {
		case RTL8380_FAMILY_ID:
			sw_w32_mask(0,
			            RTL838X_DMA_IF_INTR_MSK_TX_ALL_DONE |
			            RTL838X_DMA_IF_INTR_MSK_TX_ALL_DONE |
			            RTL838X_DMA_IF_INTR_MSK_RUNOUT |
			            FIELD_PREP(RTL838X_DMA_IF_INTR_MSK_RX_DONE, DMA_RING(r)),
			            RTL838X_DMA_IF_INTR_MSK_REG);
			break;
		case RTL8390_FAMILY_ID:
			sw_w32_mask(0,
			            RTL839X_DMA_IF_INTR_MSK_TX_ALL_DONE |
			            RTL839X_DMA_IF_INTR_MSK_TX_ALL_DONE |
			            RTL839X_DMA_IF_INTR_MSK_RX_RUNOUT |
			            FIELD_PREP(RTL839X_DMA_IF_INTR_MSK_RX_DONE, DMA_RING(r)),
			            RTL839X_DMA_IF_INTR_MSK_REG);
			break;
		case RTL9300_FAMILY_ID:
			sw_w32(RTL930X_DMA_IF_INTR_RX_DONE_MSK_DONE, RTL930X_DMA_IF_INTR_RX_DONE_MSK_REG);
			break;
		case RTL9310_FAMILY_ID:
			sw_w32(RTL931X_DMA_IF_INTR_RX_DONE_MSK_DONE, RTL931X_DMA_IF_INTR_RX_DONE_MSK_REG);
			break;
		default:
			pr_err("%s: Unsupported chip family: %d\n", __func__, priv->family_id);
		}
	}

	return work_done;
}


static void rtl838x_validate(struct phylink_config *config,
			 unsigned long *supported,
			 struct phylink_link_state *state)
{
	__ETHTOOL_DECLARE_LINK_MODE_MASK(mask) = { 0, };

	pr_debug("In %s\n", __func__);

	if (!phy_interface_mode_is_rgmii(state->interface) &&
	    state->interface != PHY_INTERFACE_MODE_1000BASEX &&
	    state->interface != PHY_INTERFACE_MODE_MII &&
	    state->interface != PHY_INTERFACE_MODE_REVMII &&
	    state->interface != PHY_INTERFACE_MODE_GMII &&
	    state->interface != PHY_INTERFACE_MODE_QSGMII &&
	    state->interface != PHY_INTERFACE_MODE_INTERNAL &&
	    state->interface != PHY_INTERFACE_MODE_SGMII) {
		bitmap_zero(supported, __ETHTOOL_LINK_MODE_MASK_NBITS);
		pr_err("Unsupported interface: %d\n", state->interface);
		return;
	}

	/* Allow all the expected bits */
	phylink_set(mask, Autoneg);
	phylink_set_port_modes(mask);
	phylink_set(mask, Pause);
	phylink_set(mask, Asym_Pause);

	/* With the exclusion of MII and Reverse MII, we support Gigabit,
	 * including Half duplex
	 */
	if (state->interface != PHY_INTERFACE_MODE_MII &&
	    state->interface != PHY_INTERFACE_MODE_REVMII) {
		phylink_set(mask, 1000baseT_Full);
		phylink_set(mask, 1000baseT_Half);
	}

	phylink_set(mask, 10baseT_Half);
	phylink_set(mask, 10baseT_Full);
	phylink_set(mask, 100baseT_Half);
	phylink_set(mask, 100baseT_Full);

	bitmap_and(supported, supported, mask,
		   __ETHTOOL_LINK_MODE_MASK_NBITS);
	bitmap_and(state->advertising, state->advertising, mask,
		   __ETHTOOL_LINK_MODE_MASK_NBITS);
}


static void rtl838x_mac_config(struct phylink_config *config,
			       unsigned int mode,
			       const struct phylink_link_state *state)
{
	/* This is only being called for the master device,
	 * i.e. the CPU-Port. We don't need to do anything.
	 */

	pr_info("In %s, mode %x\n", __func__, mode);
}

static void rtl838x_mac_an_restart(struct phylink_config *config)
{
	struct net_device *dev = container_of(config->dev, struct net_device, dev);
	struct rtl838x_eth_priv *priv = netdev_priv(dev);

	/* This works only on RTL838x chips */
	if (priv->family_id != RTL8380_FAMILY_ID)
		return;

	pr_debug("In %s\n", __func__);
	/* Restart by disabling and re-enabling link */
	sw_w32(0x6192D, priv->r->mac_force_mode_ctrl + priv->cpu_port * 4);
	msleep(20);
	sw_w32(0x6192F, priv->r->mac_force_mode_ctrl + priv->cpu_port * 4);
}

static void rtl838x_mac_pcs_get_state(struct phylink_config *config,
				  struct phylink_link_state *state)
{
	u32 speed;
	struct net_device *dev = container_of(config->dev, struct net_device, dev);
	struct rtl838x_eth_priv *priv = netdev_priv(dev);
	int port = priv->cpu_port;

	pr_info("In %s\n", __func__);

	state->link = priv->r->get_mac_link_sts(port) ? 1 : 0;
	state->duplex = priv->r->get_mac_link_dup_sts(port) ? 1 : 0;

	pr_info("%s link status is %d\n", __func__, state->link);
	speed = priv->r->get_mac_link_spd_sts(port);
	switch (speed) {
	case 0:
		state->speed = SPEED_10;
		break;
	case 1:
		state->speed = SPEED_100;
		break;
	case 2:
		state->speed = SPEED_1000;
		break;
	case 5:
		state->speed = SPEED_2500;
		break;
	case 6:
		state->speed = SPEED_5000;
		break;
	case 4:
		state->speed = SPEED_10000;
		break;
	default:
		state->speed = SPEED_UNKNOWN;
		break;
	}

	state->pause &= ~(MLO_PAUSE_TXRX_MASK);
	if (priv->r->get_mac_rx_pause_sts(port))
		state->pause |= MLO_PAUSE_RX;
	if (priv->r->get_mac_tx_pause_sts(port))
		state->pause |= MLO_PAUSE_TX;
}

static void rtl838x_mac_link_down(struct phylink_config *config,
				  unsigned int mode,
				  phy_interface_t interface)
{
	struct net_device *dev = container_of(config->dev, struct net_device, dev);
	struct rtl838x_eth_priv *priv = netdev_priv(dev);

	pr_debug("In %s\n", __func__);
	/* Stop TX/RX to port */
	sw_w32_mask(0x03, 0, priv->r->mac_port_ctrl(priv->cpu_port));
}

static void rtl838x_mac_link_up(struct phylink_config *config,
			    struct phy_device *phy, unsigned int mode,
			    phy_interface_t interface, int speed, int duplex,
			    bool tx_pause, bool rx_pause)
{
	struct net_device *dev = container_of(config->dev, struct net_device, dev);
	struct rtl838x_eth_priv *priv = netdev_priv(dev);

	pr_debug("In %s\n", __func__);
	/* Restart TX/RX to port */
	sw_w32_mask(0, 0x03, priv->r->mac_port_ctrl(priv->cpu_port));
}

static void rtl83xx_get_mac_hw(struct net_device *dev, u8 mac[])
{
	struct rtl838x_eth_priv *priv = netdev_priv(dev);
	u32 reg;

	reg = sw_r32(priv->r->mac);
	mac[0] = (reg >> 8) & GENMASK(7, 0);
	mac[1] = (reg >> 0) & GENMASK(7, 0);
	reg = sw_r32(priv->r->mac + 4);
	mac[2] = (reg >> 24) & GENMASK(7, 0);
	mac[3] = (reg >> 16) & GENMASK(7, 0);
	mac[4] = (reg >> 8) & GENMASK(7, 0);
	mac[5] = (reg >> 0) & GENMASK(7, 0);
}

static void rtl838x_set_mac_hw(struct net_device *dev, u8 *mac)
{
	struct rtl838x_eth_priv *priv = netdev_priv(dev);
	unsigned long flags;

	spin_lock_irqsave(&priv->lock, flags);
	pr_debug("In %s\n", __func__);
	sw_w32((mac[0] << 8) | mac[1], priv->r->mac);
	sw_w32((mac[2] << 24) | (mac[3] << 16) | (mac[4] << 8) | mac[5], priv->r->mac + 4);

	if (priv->family_id == RTL8380_FAMILY_ID) {
		/* 2 more registers, ALE/MAC block */
		sw_w32((mac[0] << 8) | mac[1], RTL838X_MAC_ALE);
		sw_w32((mac[2] << 24) | (mac[3] << 16) | (mac[4] << 8) | mac[5],
		       (RTL838X_MAC_ALE + 4));

		sw_w32((mac[0] << 8) | mac[1], RTL838X_MAC2);
		sw_w32((mac[2] << 24) | (mac[3] << 16) | (mac[4] << 8) | mac[5],
		       RTL838X_MAC2 + 4);
	}
	spin_unlock_irqrestore(&priv->lock, flags);
}

static int rtl838x_set_mac_address(struct net_device *dev, void *p)
{
	struct rtl838x_eth_priv *priv = netdev_priv(dev);
	const struct sockaddr *addr = p;
	u8 *mac = (u8 *) (addr->sa_data);

	if (!is_valid_ether_addr(addr->sa_data))
		return -EADDRNOTAVAIL;

	memcpy(dev->dev_addr, addr->sa_data, ETH_ALEN);
	rtl838x_set_mac_hw(dev, mac);

	pr_info("Using MAC %08x%08x\n", sw_r32(priv->r->mac), sw_r32(priv->r->mac + 4));

	return 0;
}

static int rtl8390_init_mac(struct rtl838x_eth_priv *priv)
{
	/* We will need to set-up EEE and the egress-rate limitation */
	return 0;
}

static int rtl8380_init_mac(struct rtl838x_eth_priv *priv)
{
	if (priv->family_id == RTL8390_FAMILY_ID)
		return rtl8390_init_mac(priv);

	/* At present we do not know how to set up EEE on any other SoC than RTL8380 */
	if (priv->family_id != RTL8380_FAMILY_ID)
		return 0;

	pr_info("%s\n", __func__);
	/* fix timer for EEE */
	sw_w32(0x5001411, RTL838X_EEE_TX_TIMER_GIGA_CTRL);
	sw_w32(0x5001417, RTL838X_EEE_TX_TIMER_GELITE_CTRL);

	/* Init VLAN. TODO: Understand what is being done, here */
	if (priv->id == RTL8383_FAMILY_ID) {
		for (int i = 0; i <= RTL838X_PORT_CNT; i++)
			sw_w32(0, 0xd57c + i * 0x80);
	}
	if (priv->id == RTL8380_FAMILY_ID) {
		for (int i = 8; i <= RTL838X_PORT_CNT; i++)
			sw_w32(0, 0xd57c + i * 0x80);
	}

	return 0;
}

static int rtl838x_get_link_ksettings(struct net_device *ndev,
				      struct ethtool_link_ksettings *cmd)
{
	struct rtl838x_eth_priv *priv = netdev_priv(ndev);

	pr_debug("%s called\n", __func__);

	return phylink_ethtool_ksettings_get(priv->phylink, cmd);
}

static int rtl838x_set_link_ksettings(struct net_device *ndev,
				      const struct ethtool_link_ksettings *cmd)
{
	struct rtl838x_eth_priv *priv = netdev_priv(ndev);

	pr_debug("%s called\n", __func__);

	return phylink_ethtool_ksettings_set(priv->phylink, cmd);
}

static int rtl838x_mdio_read_paged(struct mii_bus *bus, int mii_id, u16 page, int regnum)
{
	u32 val;
	int err;
	struct rtl838x_eth_priv *priv = bus->priv;

	if (mii_id >= 24 && mii_id <= 27 && priv->id == RTL8380_FAMILY_ID)
		return rtl838x_read_sds_phy(mii_id, regnum);

	if (regnum & (MII_ADDR_C45 | MII_ADDR_C22_MMD)) {
		err = rtl838x_read_mmd_phy(mii_id,
					   mdiobus_c45_devad(regnum),
					   regnum, &val);
		pr_debug("MMD: %d dev %x register %x read %x, err %d\n", mii_id,
			 mdiobus_c45_devad(regnum), mdiobus_c45_regad(regnum),
			 val, err);
	} else {
		pr_debug("PHY: %d register %x read %x, err %d\n", mii_id, regnum, val, err);
		err = rtl838x_read_phy(mii_id, page, regnum, &val);
	}
	if (err)
		return err;

	return val;
}

static int rtl838x_mdio_read(struct mii_bus *bus, int mii_id, int regnum)
{
	return rtl838x_mdio_read_paged(bus, mii_id, 0, regnum);
}

static int rtl839x_mdio_read_paged(struct mii_bus *bus, int mii_id, u16 page, int regnum)
{
	u32 val;
	int err;
	struct rtl838x_eth_priv *priv = bus->priv;

	if (mii_id >= 48 && mii_id <= 49 && priv->id == RTL8393_FAMILY_ID)
		return rtl839x_read_sds_phy(mii_id, regnum);

	if (regnum & (MII_ADDR_C45 | MII_ADDR_C22_MMD)) {
		err = rtl839x_read_mmd_phy(mii_id,
					   mdiobus_c45_devad(regnum),
					   regnum, &val);
		pr_debug("MMD: %d dev %x register %x read %x, err %d\n", mii_id,
			 mdiobus_c45_devad(regnum), mdiobus_c45_regad(regnum),
			 val, err);
	} else {
		err = rtl839x_read_phy(mii_id, page, regnum, &val);
		pr_debug("PHY: %d register %x read %x, err %d\n", mii_id, regnum, val, err);
	}

	if (err)
		return err;

	return val;
}

static int rtl839x_mdio_read(struct mii_bus *bus, int mii_id, int regnum)
{
	return rtl839x_mdio_read_paged(bus, mii_id, 0, regnum);
}

static int rtl930x_mdio_read_paged(struct mii_bus *bus, int mii_id, u16 page, int regnum)
{
	u32 val;
	int err;
	struct rtl838x_eth_priv *priv = bus->priv;

	if (priv->phy_is_internal[mii_id])
		return rtl930x_read_sds_phy(priv->sds_id[mii_id], page, regnum);

	if (regnum & (MII_ADDR_C45 | MII_ADDR_C22_MMD)) {
		err = rtl930x_read_mmd_phy(mii_id,
					   mdiobus_c45_devad(regnum),
					   regnum, &val);
		pr_debug("MMD: %d dev %x register %x read %x, err %d\n", mii_id,
			 mdiobus_c45_devad(regnum), mdiobus_c45_regad(regnum),
			 val, err);
	} else {
		err = rtl930x_read_phy(mii_id, page, regnum, &val);
		pr_debug("PHY: %d register %x read %x, err %d\n", mii_id, regnum, val, err);
	}

	if (err)
		return err;

	return val;
}

static int rtl930x_mdio_read(struct mii_bus *bus, int mii_id, int regnum)
{
	return rtl930x_mdio_read_paged(bus, mii_id, 0, regnum);
}

static int rtl931x_mdio_read_paged(struct mii_bus *bus, int mii_id, u16 page, int regnum)
{
	u32 val;
	int err, v;
	struct rtl838x_eth_priv *priv = bus->priv;

	pr_debug("%s: In here, port %d\n", __func__, mii_id);
	if (priv->phy_is_internal[mii_id]) {
		v = rtl931x_read_sds_phy(priv->sds_id[mii_id], page, regnum);
		if (v < 0) {
			err = v;
		} else {
			err = 0;
			val = v;
		}
	} else {
		if (regnum & (MII_ADDR_C45 | MII_ADDR_C22_MMD)) {
			err = rtl931x_read_mmd_phy(mii_id,
						   mdiobus_c45_devad(regnum),
						   regnum, &val);
			pr_debug("MMD: %d dev %x register %x read %x, err %d\n", mii_id,
				 mdiobus_c45_devad(regnum), mdiobus_c45_regad(regnum),
				 val, err);
		} else {
			err = rtl931x_read_phy(mii_id, page, regnum, &val);
			pr_debug("PHY: %d register %x read %x, err %d\n", mii_id, regnum, val, err);
		}
	}

	if (err)
		return err;

	return val;
}

static int rtl931x_mdio_read(struct mii_bus *bus, int mii_id, int regnum)
{
	return rtl931x_mdio_read_paged(bus, mii_id, 0, regnum);
}

static int rtl838x_mdio_write_paged(struct mii_bus *bus, int mii_id, u16 page,
				    int regnum, u16 value)
{
	u32 offset = 0;
	struct rtl838x_eth_priv *priv = bus->priv;
	int err;

	if (mii_id >= 24 && mii_id <= 27 && priv->id == RTL8380_FAMILY_ID) {
		if (mii_id == 26)
			offset = 0x100;
		sw_w32(value, RTL838X_SDS4_FIB_REG0 + offset + (regnum << 2));
		return 0;
	}

	if (regnum & (MII_ADDR_C45 | MII_ADDR_C22_MMD)) {
		err = rtl838x_write_mmd_phy(mii_id, mdiobus_c45_devad(regnum),
					    regnum, value);
		pr_debug("MMD: %d dev %x register %x write %x, err %d\n", mii_id,
			 mdiobus_c45_devad(regnum), mdiobus_c45_regad(regnum),
			 value, err);

		return err;
	}
	err = rtl838x_write_phy(mii_id, page, regnum, value);
	pr_debug("PHY: %d register %x write %x, err %d\n", mii_id, regnum, value, err);

	return err;
}

static int rtl838x_mdio_write(struct mii_bus *bus, int mii_id,
			      int regnum, u16 value)
{
	return rtl838x_mdio_write_paged(bus, mii_id, 0, regnum, value);
}

static int rtl839x_mdio_write_paged(struct mii_bus *bus, int mii_id, u16 page,
				    int regnum, u16 value)
{
	struct rtl838x_eth_priv *priv = bus->priv;
	int err;

	if (mii_id >= 48 && mii_id <= 49 && priv->id == RTL8393_FAMILY_ID)
		return rtl839x_write_sds_phy(mii_id, regnum, value);

	if (regnum & (MII_ADDR_C45 | MII_ADDR_C22_MMD)) {
		err = rtl839x_write_mmd_phy(mii_id, mdiobus_c45_devad(regnum),
					    regnum, value);
		pr_debug("MMD: %d dev %x register %x write %x, err %d\n", mii_id,
			 mdiobus_c45_devad(regnum), mdiobus_c45_regad(regnum),
			 value, err);

		return err;
	}

	err = rtl839x_write_phy(mii_id, page, regnum, value);
	pr_debug("PHY: %d register %x write %x, err %d\n", mii_id, regnum, value, err);

	return err;
}

static int rtl839x_mdio_write(struct mii_bus *bus, int mii_id,
			      int regnum, u16 value)
{
	return rtl839x_mdio_write_paged(bus, mii_id, 0, regnum, value);
}

static int rtl930x_mdio_write_paged(struct mii_bus *bus, int mii_id, u16 page,
				    int regnum, u16 value)
{
	struct rtl838x_eth_priv *priv = bus->priv;
	int err;

	if (priv->phy_is_internal[mii_id])
		return rtl930x_write_sds_phy(priv->sds_id[mii_id], page, regnum, value);

	if (regnum & (MII_ADDR_C45 | MII_ADDR_C22_MMD))
		return rtl930x_write_mmd_phy(mii_id, mdiobus_c45_devad(regnum),
					     regnum, value);

	err = rtl930x_write_phy(mii_id, page, regnum, value);
	pr_debug("PHY: %d register %x write %x, err %d\n", mii_id, regnum, value, err);

	return err;
}

static int rtl930x_mdio_write(struct mii_bus *bus, int mii_id,
			      int regnum, u16 value)
{
	return rtl930x_mdio_write_paged(bus, mii_id, 0, regnum, value);
}

static int rtl931x_mdio_write_paged(struct mii_bus *bus, int mii_id, u16 page,
				    int regnum, u16 value)
{
	struct rtl838x_eth_priv *priv = bus->priv;
	int err;

	if (priv->phy_is_internal[mii_id])
		return rtl931x_write_sds_phy(priv->sds_id[mii_id], page, regnum, value);

	if (regnum & (MII_ADDR_C45 | MII_ADDR_C22_MMD)) {
		err = rtl931x_write_mmd_phy(mii_id, mdiobus_c45_devad(regnum),
					    regnum, value);
		pr_debug("MMD: %d dev %x register %x write %x, err %d\n", mii_id,
			 mdiobus_c45_devad(regnum), mdiobus_c45_regad(regnum),
			 value, err);

		return err;
	}

	err = rtl931x_write_phy(mii_id, page, regnum, value);
	pr_debug("PHY: %d register %x write %x, err %d\n", mii_id, regnum, value, err);

	return err;
}

static int rtl931x_mdio_write(struct mii_bus *bus, int mii_id,
			      int regnum, u16 value)
{
	return rtl931x_mdio_write_paged(bus, mii_id, 0, regnum, value);
}

static int rtl838x_mdio_reset(struct mii_bus *bus)
{
	pr_debug("%s called\n", __func__);
	/* Disable MAC polling the PHY so that we can start configuration */
	sw_w32(0x00000000, RTL838X_SMI_POLL_CTRL);

	/* Enable PHY control via SoC */
	sw_w32_mask(0, 1 << 15, RTL838X_SMI_GLB_CTRL);

	/* Probably should reset all PHYs here... */
	return 0;
}

static int rtl839x_mdio_reset(struct mii_bus *bus)
{
	return 0;

	pr_debug("%s called\n", __func__);
	/* BUG: The following does not work, but should! */
	/* Disable MAC polling the PHY so that we can start configuration */
	sw_w32(0x00000000, RTL839X_SMI_PORT_POLLING_CTRL);
	sw_w32(0x00000000, RTL839X_SMI_PORT_POLLING_CTRL + 4);
	/* Disable PHY polling via SoC */
	sw_w32_mask(1 << 7, 0, RTL839X_SMI_GLB_CTRL);

	/* Probably should reset all PHYs here... */
	return 0;
}

u8 mac_type_bit[RTL930X_PORT_CPU] = {0, 0, 0, 0, 2, 2, 2, 2, 4, 4, 4, 4, 6, 6, 6, 6,
				     8, 8, 8, 8, 10, 10, 10, 10, 12, 15, 18, 21};

static int rtl930x_mdio_reset(struct mii_bus *bus)
{
	struct rtl838x_eth_priv *priv = bus->priv;
	u32 c45_mask = 0;
	u32 poll_sel[2];
	u32 poll_ctrl = 0;
	u32 private_poll_mask = 0;
	u32 v;
	bool uses_usxgmii = false; /* For the Aquantia PHYs */
	bool uses_hisgmii = false; /* For the RTL8221/8226 */

	/* Mapping of port to phy-addresses on an SMI bus */
	poll_sel[0] = poll_sel[1] = 0;
	for (int i = 0; i < RTL930X_PORT_CNT; i++) {
		int pos;

		if ((priv->smi_bus[i] < 0) || (priv->smi_bus[i] > MAX_SMI_BUSSES))
			continue;
		pos = (i % 6) * 5;
		sw_w32_mask(0x1f << pos, priv->smi_addr[i] << pos,
			    RTL930X_SMI_PORT0_5_ADDR + (i / 6) * 4);

		pos = (i * 2) % 32;
		poll_sel[i / 16] |= priv->smi_bus[i] << pos;
		poll_ctrl |= BIT(20 + priv->smi_bus[i]);
	}

	/* Configure which SMI bus is behind which port number */
	sw_w32(poll_sel[0], RTL930X_SMI_PORT0_15_POLLING_SEL);
	sw_w32(poll_sel[1], RTL930X_SMI_PORT16_27_POLLING_SEL);

	/* Disable POLL_SEL for any SMI bus with a normal PHY (not RTL8295R for SFP+) */
	sw_w32_mask(poll_ctrl, 0, RTL930X_SMI_GLB_CTRL);

	/* Configure which SMI busses are polled in c45 based on a c45 PHY being on that bus */
	for (int i = 0; i < 4; i++)
		if (priv->smi_bus_isc45[i])
			c45_mask |= BIT(i + 16);

	pr_info("c45_mask: %08x\n", c45_mask);
	sw_w32_mask(0, c45_mask, RTL930X_SMI_GLB_CTRL);

	/* Set the MAC type of each port according to the PHY-interface */
	/* Values are FE: 2, GE: 3, XGE/2.5G: 0(SERDES) or 1(otherwise), SXGE: 0 */
	v = 0;
	for (int i = 0; i < RTL930X_PORT_CNT; i++) {
		switch (priv->interfaces[i]) {
		case PHY_INTERFACE_MODE_10GBASER:
			break;			/* Serdes: Value = 0 */
		case PHY_INTERFACE_MODE_HSGMII:
			private_poll_mask |= BIT(i);
			fallthrough;
		case PHY_INTERFACE_MODE_USXGMII:
			v |= BIT(mac_type_bit[i]);
			uses_usxgmii = true;
			break;
		case PHY_INTERFACE_MODE_QSGMII:
			private_poll_mask |= BIT(i);
			v |= 3 << mac_type_bit[i];
			break;
		default:
			break;
		}
	}
	sw_w32(v, RTL930X_SMI_MAC_TYPE_CTRL);

	/* Set the private polling mask for all Realtek PHYs (i.e. not the 10GBit Aquantia ones) */
	sw_w32(private_poll_mask, RTL930X_SMI_PRVTE_POLLING_CTRL);

	/* The following magic values are found in the port configuration, they seem to
	 * define different ways of polling a PHY. The below is for the Aquantia PHYs of
	 * the XGS1250 and the RTL8226 of the XGS1210
	 */
	if (uses_usxgmii) {
		sw_w32(0x01010000, RTL930X_SMI_10GPHY_POLLING_REG0_CFG);
		sw_w32(0x01E7C400, RTL930X_SMI_10GPHY_POLLING_REG9_CFG);
		sw_w32(0x01E7E820, RTL930X_SMI_10GPHY_POLLING_REG10_CFG);
	}
	if (uses_hisgmii) {
		sw_w32(0x011FA400, RTL930X_SMI_10GPHY_POLLING_REG0_CFG);
		sw_w32(0x013FA412, RTL930X_SMI_10GPHY_POLLING_REG9_CFG);
		sw_w32(0x017FA414, RTL930X_SMI_10GPHY_POLLING_REG10_CFG);
	}

	pr_debug("%s: RTL930X_SMI_GLB_CTRL %08x\n", __func__,
		 sw_r32(RTL930X_SMI_GLB_CTRL));
	pr_debug("%s: RTL930X_SMI_PORT0_15_POLLING_SEL %08x\n", __func__,
		 sw_r32(RTL930X_SMI_PORT0_15_POLLING_SEL));
	pr_debug("%s: RTL930X_SMI_PORT16_27_POLLING_SEL %08x\n", __func__,
		 sw_r32(RTL930X_SMI_PORT16_27_POLLING_SEL));
	pr_debug("%s: RTL930X_SMI_MAC_TYPE_CTRL %08x\n", __func__,
		 sw_r32(RTL930X_SMI_MAC_TYPE_CTRL));
	pr_debug("%s: RTL930X_SMI_10GPHY_POLLING_REG0_CFG %08x\n", __func__,
		 sw_r32(RTL930X_SMI_10GPHY_POLLING_REG0_CFG));
	pr_debug("%s: RTL930X_SMI_10GPHY_POLLING_REG9_CFG %08x\n", __func__,
		 sw_r32(RTL930X_SMI_10GPHY_POLLING_REG9_CFG));
	pr_debug("%s: RTL930X_SMI_10GPHY_POLLING_REG10_CFG %08x\n", __func__,
		 sw_r32(RTL930X_SMI_10GPHY_POLLING_REG10_CFG));
	pr_debug("%s: RTL930X_SMI_PRVTE_POLLING_CTRL %08x\n", __func__,
		 sw_r32(RTL930X_SMI_PRVTE_POLLING_CTRL));

	return 0;
}

static int rtl931x_mdio_reset(struct mii_bus *bus)
{
	struct rtl838x_eth_priv *priv = bus->priv;
	u32 c45_mask = 0;
	u32 poll_sel[4];
	u32 poll_ctrl = 0;
	bool mdc_on[4];

	pr_info("%s called\n", __func__);
	/* Disable port polling for configuration purposes */
	sw_w32(0, RTL931X_SMI_PORT_POLLING_CTRL);
	sw_w32(0, RTL931X_SMI_PORT_POLLING_CTRL + 4);
	msleep(100);

	mdc_on[0] = mdc_on[1] = mdc_on[2] = mdc_on[3] = false;
	/* Mapping of port to phy-addresses on an SMI bus */
	poll_sel[0] = poll_sel[1] = poll_sel[2] = poll_sel[3] = 0;
	for (int i = 0; i < RTL931X_PORT_END; i++) {
		u32 pos;

		if ((priv->smi_bus[i] < 0) || (priv->smi_bus[i] > MAX_SMI_BUSSES))
			continue;
		pos = (i % 6) * 5;
		sw_w32_mask(0x1f << pos, priv->smi_addr[i] << pos, RTL931X_SMI_PORT_ADDR + (i / 6) * 4);
		pos = (i * 2) % 32;
		poll_sel[i / 16] |= priv->smi_bus[i] << pos;
		poll_ctrl |= BIT(20 + priv->smi_bus[i]);
		mdc_on[priv->smi_bus[i]] = true;
	}

	/* Configure which SMI bus is behind which port number */
	for (int i = 0; i < 4; i++) {
		pr_info("poll sel %d, %08x\n", i, poll_sel[i]);
		sw_w32(poll_sel[i], RTL931X_SMI_PORT_POLLING_SEL + (i * 4));
	}

	/* Configure which SMI busses */
	pr_info("%s: WAS RTL931X_MAC_L2_GLOBAL_CTRL2 %08x\n", __func__, sw_r32(RTL931X_MAC_L2_GLOBAL_CTRL2));
	pr_info("c45_mask: %08x, RTL931X_SMI_GLB_CTRL0 was %X", c45_mask, sw_r32(RTL931X_SMI_GLB_CTRL0));
	for (int i = 0; i < 4; i++) {
		/* bus is polled in c45 */
		if (priv->smi_bus_isc45[i])
			c45_mask |= 0x2 << (i * 2);  /* Std. C45, non-standard is 0x3 */
		/* Enable bus access via MDC */
		if (mdc_on[i])
			sw_w32_mask(0, BIT(9 + i), RTL931X_MAC_L2_GLOBAL_CTRL2);
	}

	pr_info("%s: RTL931X_MAC_L2_GLOBAL_CTRL2 %08x\n", __func__, sw_r32(RTL931X_MAC_L2_GLOBAL_CTRL2));
	pr_info("c45_mask: %08x, RTL931X_SMI_GLB_CTRL0 was %X", c45_mask, sw_r32(RTL931X_SMI_GLB_CTRL0));

	/* We have a 10G PHY enable polling
	 * sw_w32(0x01010000, RTL931X_SMI_10GPHY_POLLING_SEL2);
	 * sw_w32(0x01E7C400, RTL931X_SMI_10GPHY_POLLING_SEL3);
	 * sw_w32(0x01E7E820, RTL931X_SMI_10GPHY_POLLING_SEL4);
	 */
	sw_w32_mask(0xff, c45_mask, RTL931X_SMI_GLB_CTRL1);

	return 0;
}

static int rtl931x_chip_init(struct rtl838x_eth_priv *priv)
{
	pr_info("In %s\n", __func__);

	/* Initialize Encapsulation memory and wait until finished */
	sw_w32(0x1, RTL931X_MEM_ENCAP_INIT_REG);
	do { } while (sw_r32(RTL931X_MEM_ENCAP_INIT_REG) & RTL931X_MEM_ENCAP_INIT_MEM_INIT);
	pr_info("%s: init ENCAP done\n", __func__);

	/* Initialize Managemen Information Base memory and wait until finished */
	sw_w32(RTL931X_MEM_MIB_INIT_MEM_RST, RTL931X_MEM_MIB_INIT_REG);
	do { } while (sw_r32(RTL931X_MEM_MIB_INIT_REG) & RTL931X_MEM_MIB_INIT_MEM_RST);
	pr_info("%s: init MIB done\n", __func__);

	/* Initialize ACL (PIE) memory and wait until finished */
	sw_w32(RTL931X_MEM_ACL_INIT_MEM_INIT, RTL931X_MEM_ACL_INIT_REG);
	do { } while (sw_r32(RTL931X_MEM_ACL_INIT_REG) & RTL931X_MEM_ACL_INIT_MEM_INIT);
	pr_info("%s: init ACL done\n", __func__);

	/* Initialize ALE memory and wait until finished */
	sw_w32(GENMASK(31, 0), RTL931X_MEM_ALE_INIT_REG(0));
	do { } while (sw_r32(RTL931X_MEM_ALE_INIT_REG(0)));
	sw_w32(GENMASK(6, 0), RTL931X_MEM_ALE_INIT_REG(32));
	sw_w32(RLT931X_MEM_RALE_INIT_MASK, RTL931X_MEM_RALE_INIT_REG);
	do { } while (sw_r32(RTL931X_MEM_RALE_INIT_REG) & RLT931X_MEM_RALE_INIT_MASK);
	pr_info("%s: init ALE done\n", __func__);

	/* Enable ESD auto recovery */
	sw_w32(RTL931X_MDX_CTRL_RSVD_ESD_AUTO_RECOVERY, RTL931X_MDX_CTRL_RSVD_REG);

	/* Init SPI, is this for thermal control or what? */
	sw_w32_mask(RTL931X_SPI_CTRL0_CLK_SEL_MASK,
	            FIELD_PREP(RTL931X_SPI_CTRL0_CLK_SEL_MASK, RTL931X_SPI_CTRL0_CLK_SEL_DIV(6)),
	            RTL931X_SPI_CTRL0_REG);

	return 0;
}

static int rtl838x_mdio_init(struct rtl838x_eth_priv *priv)
{
	struct device_node *mii_np, *dn;
	u32 pn;
	int ret;

	pr_debug("%s called\n", __func__);
	mii_np = of_get_child_by_name(priv->pdev->dev.of_node, "mdio-bus");

	if (!mii_np) {
		dev_err(&priv->pdev->dev, "no %s child node found", "mdio-bus");
		return -ENODEV;
	}

	if (!of_device_is_available(mii_np)) {
		ret = -ENODEV;
		goto err_put_node;
	}

	priv->mii_bus = devm_mdiobus_alloc(&priv->pdev->dev);
	if (!priv->mii_bus) {
		ret = -ENOMEM;
		goto err_put_node;
	}

	switch(priv->family_id) {
	case RTL8380_FAMILY_ID:
		priv->mii_bus->name = "rtl838x-eth-mdio";
		priv->mii_bus->read = rtl838x_mdio_read;
		priv->mii_bus->read_paged = rtl838x_mdio_read_paged;
		priv->mii_bus->write = rtl838x_mdio_write;
		priv->mii_bus->write_paged = rtl838x_mdio_write_paged;
		priv->mii_bus->reset = rtl838x_mdio_reset;
		break;
	case RTL8390_FAMILY_ID:
		priv->mii_bus->name = "rtl839x-eth-mdio";
		priv->mii_bus->read = rtl839x_mdio_read;
		priv->mii_bus->read_paged = rtl839x_mdio_read_paged;
		priv->mii_bus->write = rtl839x_mdio_write;
		priv->mii_bus->write_paged = rtl839x_mdio_write_paged;
		priv->mii_bus->reset = rtl839x_mdio_reset;
		break;
	case RTL9300_FAMILY_ID:
		priv->mii_bus->name = "rtl930x-eth-mdio";
		priv->mii_bus->read = rtl930x_mdio_read;
		priv->mii_bus->read_paged = rtl930x_mdio_read_paged;
		priv->mii_bus->write = rtl930x_mdio_write;
		priv->mii_bus->write_paged = rtl930x_mdio_write_paged;
		priv->mii_bus->reset = rtl930x_mdio_reset;
		priv->mii_bus->probe_capabilities = MDIOBUS_C22_C45;
		break;
	case RTL9310_FAMILY_ID:
		priv->mii_bus->name = "rtl931x-eth-mdio";
		priv->mii_bus->read = rtl931x_mdio_read;
		priv->mii_bus->read_paged = rtl931x_mdio_read_paged;
		priv->mii_bus->write = rtl931x_mdio_write;
		priv->mii_bus->write_paged = rtl931x_mdio_write_paged;
		priv->mii_bus->reset = rtl931x_mdio_reset;
		priv->mii_bus->probe_capabilities = MDIOBUS_C22_C45;
		break;
	}
	priv->mii_bus->access_capabilities = MDIOBUS_ACCESS_C22_MMD;
	priv->mii_bus->priv = priv;
	priv->mii_bus->parent = &priv->pdev->dev;

	for_each_node_by_name(dn, "ethernet-phy") {
		u32 smi_addr[2];

		if (of_property_read_u32(dn, "reg", &pn)) {
			pr_err("%s: missing reg property on port %d\n", __func__, pn);
			continue;
		}

		if (pn >= MAX_PORTS) {
			pr_err("%s: illegal port number %d\n", __func__, pn);
			continue;
		}

		priv->phy_is_internal[pn] = of_property_read_bool(dn, "phy-is-integrated");

		if (of_property_read_u32(dn, "sds", &priv->sds_id[pn]))
			priv->sds_id[pn] = -1;
		else {
			pr_info("set sds port %d to %d\n", pn, priv->sds_id[pn]);
		}

		if (of_property_read_u32_array(dn, "rtl9300,smi-address", &smi_addr[0], 2)) {
			/* Integrated PHYs associated to a SerDes do not have an smi_bus */
			if (priv->phy_is_internal[pn] && priv->sds_id[pn] >= 0) {
				priv->smi_bus[pn] = -1;
			/* PHYs whether integrated or not, not associated to an SDS use an smi_bus */
			} else {
				/* For RTL83xx, PHY-id is port ID on smi_bus 0 */
				priv->smi_bus[pn] = 0;
				priv->smi_addr[pn] = pn;
			}
		} else {
			priv->smi_bus[pn] = smi_addr[0];
			priv->smi_addr[pn] = smi_addr[1];
		}

		if (priv->smi_bus[pn] > MAX_SMI_BUSSES) {
			pr_err("%s: illegal SMI bus number %d\n", __func__, priv->smi_bus[pn]);
			continue;
		}

		if (priv->smi_bus[pn] >= 0)
			priv->smi_bus_isc45[priv->smi_bus[pn]] = !!of_device_is_compatible(dn, "ethernet-phy-ieee802.3-c45");
	}

	dn = of_find_compatible_node(NULL, NULL, "realtek,rtl83xx-switch");
	if (!dn) {
		dev_err(&priv->pdev->dev, "No RTL switch node in DTS\n");
		return -ENODEV;
	}

	for_each_node_by_name(dn, "port") {
		if (of_property_read_u32(dn, "reg", &pn))
			continue;
		pr_debug("%s Looking at port %d\n", __func__, pn);
		if (pn > priv->cpu_port)
			continue;
		if (of_get_phy_mode(dn, &priv->interfaces[pn]))
			priv->interfaces[pn] = PHY_INTERFACE_MODE_NA;
		pr_debug("%s phy mode of port %d is %s\n", __func__, pn, phy_modes(priv->interfaces[pn]));
	}

	snprintf(priv->mii_bus->id, MII_BUS_ID_SIZE, "%pOFn", mii_np);
	ret = of_mdiobus_register(priv->mii_bus, mii_np);

err_put_node:
	of_node_put(mii_np);

	return ret;
}

static int rtl838x_mdio_remove(struct rtl838x_eth_priv *priv)
{
	pr_debug("%s called\n", __func__);
	if (!priv->mii_bus)
		return 0;

	mdiobus_unregister(priv->mii_bus);
	mdiobus_free(priv->mii_bus);

	return 0;
}

static netdev_features_t rtl838x_fix_features(struct net_device *dev,
					  netdev_features_t features)
{
	return features;
}

static int rtl838x_set_features(struct net_device *dev, netdev_features_t features)
{
	struct rtl838x_eth_priv *priv = netdev_priv(dev);

	if ((features ^ dev->features) & NETIF_F_RXCSUM) {
		if (!(features & NETIF_F_RXCSUM))
			sw_w32_mask(BIT(3), 0, priv->r->mac_port_ctrl(priv->cpu_port));
		else
			sw_w32_mask(0, BIT(3), priv->r->mac_port_ctrl(priv->cpu_port));
	}

	return 0;
}

static int rtl839x_set_features(struct net_device *dev, netdev_features_t features)
{
	struct rtl838x_eth_priv *priv = netdev_priv(dev);

	if ((features ^ dev->features) & NETIF_F_RXCSUM) {
		if (!(features & NETIF_F_RXCSUM))
			sw_w32_mask(BIT(3), 0, priv->r->mac_port_ctrl(priv->cpu_port));
		else
			sw_w32_mask(0, BIT(3), priv->r->mac_port_ctrl(priv->cpu_port));
	}

	return 0;
}

static int rtl930x_set_features(struct net_device *dev, netdev_features_t features)
{
	struct rtl838x_eth_priv *priv = netdev_priv(dev);

	if ((features ^ dev->features) & NETIF_F_RXCSUM) {
		if (!(features & NETIF_F_RXCSUM))
			sw_w32_mask(BIT(4), 0, priv->r->mac_port_ctrl(priv->cpu_port));
		else
			sw_w32_mask(0, BIT(4), priv->r->mac_port_ctrl(priv->cpu_port));
	}

	return 0;
}

static int rtl931x_set_features(struct net_device *dev, netdev_features_t features)
{
	/* RXCSUM not supported */

	return 0;
}

static const struct net_device_ops rtl838x_eth_netdev_ops = {
	.ndo_open = rtl838x_eth_open,
	.ndo_stop = rtl838x_eth_stop,
	.ndo_start_xmit = rtl838x_eth_tx,
	.ndo_select_queue = rtl83xx_pick_tx_queue,
	.ndo_set_mac_address = rtl838x_set_mac_address,
	.ndo_validate_addr = eth_validate_addr,
	.ndo_set_rx_mode = rtl838x_eth_set_multicast_list,
	.ndo_tx_timeout = rtl838x_eth_tx_timeout,
	.ndo_set_features = rtl838x_set_features,
	.ndo_fix_features = rtl838x_fix_features,
	.ndo_setup_tc = rtl83xx_setup_tc,
};

static const struct net_device_ops rtl839x_eth_netdev_ops = {
	.ndo_open = rtl838x_eth_open,
	.ndo_stop = rtl838x_eth_stop,
	.ndo_start_xmit = rtl838x_eth_tx,
	.ndo_select_queue = rtl83xx_pick_tx_queue,
	.ndo_set_mac_address = rtl838x_set_mac_address,
	.ndo_validate_addr = eth_validate_addr,
	.ndo_set_rx_mode = rtl839x_eth_set_multicast_list,
	.ndo_tx_timeout = rtl838x_eth_tx_timeout,
	.ndo_set_features = rtl839x_set_features,
	.ndo_fix_features = rtl838x_fix_features,
	.ndo_setup_tc = rtl83xx_setup_tc,
};

static const struct net_device_ops rtl930x_eth_netdev_ops = {
	.ndo_open = rtl838x_eth_open,
	.ndo_stop = rtl838x_eth_stop,
	.ndo_start_xmit = rtl838x_eth_tx,
	.ndo_select_queue = rtl93xx_pick_tx_queue,
	.ndo_set_mac_address = rtl838x_set_mac_address,
	.ndo_validate_addr = eth_validate_addr,
	.ndo_set_rx_mode = rtl930x_eth_set_multicast_list,
	.ndo_tx_timeout = rtl838x_eth_tx_timeout,
	.ndo_set_features = rtl930x_set_features,
	.ndo_fix_features = rtl838x_fix_features,
	.ndo_setup_tc = rtl83xx_setup_tc,
};

static const struct net_device_ops rtl931x_eth_netdev_ops = {
	.ndo_open = rtl838x_eth_open,
	.ndo_stop = rtl838x_eth_stop,
	.ndo_start_xmit = rtl838x_eth_tx,
	.ndo_select_queue = rtl93xx_pick_tx_queue,
	.ndo_set_mac_address = rtl838x_set_mac_address,
	.ndo_validate_addr = eth_validate_addr,
	.ndo_set_rx_mode = rtl931x_eth_set_multicast_list,
	.ndo_tx_timeout = rtl838x_eth_tx_timeout,
	.ndo_set_features = rtl931x_set_features,
	.ndo_fix_features = rtl838x_fix_features,
};

static const struct phylink_mac_ops rtl838x_phylink_ops = {
	.validate = rtl838x_validate,
	.mac_pcs_get_state = rtl838x_mac_pcs_get_state,
	.mac_an_restart = rtl838x_mac_an_restart,
	.mac_config = rtl838x_mac_config,
	.mac_link_down = rtl838x_mac_link_down,
	.mac_link_up = rtl838x_mac_link_up,
};

static const struct ethtool_ops rtl838x_ethtool_ops = {
	.get_link_ksettings     = rtl838x_get_link_ksettings,
	.set_link_ksettings     = rtl838x_set_link_ksettings,
};

static int __init rtl838x_eth_probe(struct platform_device *pdev)
{
	struct net_device *dev;
	struct device_node *dn = pdev->dev.of_node;
	struct rtl838x_eth_priv *priv;
	struct resource *res, *mem;
	phy_interface_t phy_mode;
	struct phylink *phylink;
	int err = 0, rxrings, rxringlen;
	struct ring_b *ring;

	pr_info("Probing RTL838X eth device pdev: %x, dev: %x\n",
		(u32)pdev, (u32)(&(pdev->dev)));

	if (!dn) {
		dev_err(&pdev->dev, "No DT found\n");
		return -EINVAL;
	}

	switch (soc_info.family) {
	case RTL8380_FAMILY_ID:
		rxrings = RTL838X_DMA_IF_RX_RING_MAX;
		rxringlen = RTL838X_DMA_IF_RX_RING_LEN;
		break;
	case RTL8390_FAMILY_ID:
		rxrings = RTL839X_DMA_IF_RX_RING_MAX;
		rxringlen = RTL839X_DMA_IF_RX_RING_LEN;
		break;
	case RTL9300_FAMILY_ID:
		rxrings = RTL930X_DMA_IF_RX_RING_MAX;
		rxringlen = RTL930X_DMA_IF_RX_RING_LEN;
		break;
	case RTL9310_FAMILY_ID:
		rxrings = RTL931X_DMA_IF_RX_RING_MAX;
		rxringlen = RTL931X_DMA_IF_RX_RING_LEN;
		break;
	default:
		pr_err("%s: Unsupported chip family: %d\n", __func__, soc_info.family);
		break;
	};

	dev = alloc_etherdev_mqs(sizeof(struct rtl838x_eth_priv), RTL838X_DMA_IF_TX_RING_MAX, rxrings);
	if (!dev) {
		err = -ENOMEM;
		goto err_free;
	}
	SET_NETDEV_DEV(dev, &pdev->dev);
	priv = netdev_priv(dev);

	/* obtain buffer memory space */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res) {
		mem = devm_request_mem_region(&pdev->dev, res->start,
			resource_size(res), res->name);
		if (!mem) {
			dev_err(&pdev->dev, "cannot request memory space\n");
			err = -ENXIO;
			goto err_free;
		}

		dev->mem_start = mem->start;
		dev->mem_end   = mem->end;
	} else {
		dev_err(&pdev->dev, "cannot request IO resource\n");
		err = -ENXIO;
		goto err_free;
	}

	/* Allocate buffer memory */
	priv->membase = dmam_alloc_coherent(&pdev->dev, rxrings * rxringlen * RING_BUFFER +
	                                    sizeof(struct ring_b) + sizeof(struct notify_b),
	                                    (void *)&dev->mem_start, GFP_KERNEL);
	if (!priv->membase) {
		dev_err(&pdev->dev, "cannot allocate DMA buffer\n");
		err = -ENOMEM;
		goto err_free;
	}

	/* Allocate ring-buffer space at the end of the allocated memory */
	ring = priv->membase;
	ring->rx_space = priv->membase + sizeof(struct ring_b) + sizeof(struct notify_b);

	spin_lock_init(&priv->lock);

	dev->ethtool_ops = &rtl838x_ethtool_ops;
	dev->min_mtu = ETH_ZLEN;
	dev->max_mtu = 1536;
	dev->features = NETIF_F_RXCSUM | NETIF_F_HW_CSUM;
	dev->hw_features = NETIF_F_RXCSUM;

	priv->id = soc_info.id;
	priv->family_id = soc_info.family;
	if (priv->id) {
		pr_info("Found SoC ID: %4x: %s, family %x\n",
			priv->id, soc_info.name, priv->family_id);
	} else {
		pr_err("Unknown chip id (%04x)\n", priv->id);
		return -ENODEV;
	}

	switch (priv->family_id) {
	case RTL8380_FAMILY_ID:
		priv->cpu_port = RTL838X_PORT_CPU;
		priv->r = &rtl838x_reg;
		dev->netdev_ops = &rtl838x_eth_netdev_ops;
		break;
	case RTL8390_FAMILY_ID:
		priv->cpu_port = RTL839X_PORT_CPU;
		priv->r = &rtl839x_reg;
		dev->netdev_ops = &rtl839x_eth_netdev_ops;
		break;
	case RTL9300_FAMILY_ID:
		priv->cpu_port = RTL930X_PORT_CPU;
		priv->r = &rtl930x_reg;
		dev->netdev_ops = &rtl930x_eth_netdev_ops;
		break;
	case RTL9310_FAMILY_ID:
		priv->cpu_port = RTL931X_PORT_CPU;
		priv->r = &rtl931x_reg;
		dev->netdev_ops = &rtl931x_eth_netdev_ops;
		rtl931x_chip_init(priv);
		break;
	default:
		pr_err("Unknown SoC family\n");
		return -ENODEV;
	}
	priv->rxringlen = rxringlen;
	priv->rxrings = rxrings;

	/* Obtain device IRQ number */
	dev->irq = platform_get_irq(pdev, 0);
	if (dev->irq < 0) {
		dev_err(&pdev->dev, "cannot obtain network-device IRQ\n");
		goto err_free;
	}

	err = devm_request_irq(&pdev->dev, dev->irq, priv->r->net_irq,
			       IRQF_SHARED, dev->name, dev);
	if (err) {
		dev_err(&pdev->dev, "%s: could not acquire interrupt: %d\n",
			   __func__, err);
		goto err_free;
	}

	rtl8380_init_mac(priv);

	/* Try to get mac address in the following order:
	 * 1) from device tree data
	 * 2) from internal registers set by bootloader
	 */
	of_get_mac_address(pdev->dev.of_node, dev->dev_addr);
	if (is_valid_ether_addr(dev->dev_addr)) {
		rtl838x_set_mac_hw(dev, (u8 *)dev->dev_addr);
	} else {
		dev->dev_addr[0] = (sw_r32(priv->r->mac) >> 8) & 0xff;
		dev->dev_addr[1] = sw_r32(priv->r->mac) & 0xff;
		dev->dev_addr[2] = (sw_r32(priv->r->mac + 4) >> 24) & 0xff;
		dev->dev_addr[3] = (sw_r32(priv->r->mac + 4) >> 16) & 0xff;
		dev->dev_addr[4] = (sw_r32(priv->r->mac + 4) >> 8) & 0xff;
		dev->dev_addr[5] = sw_r32(priv->r->mac + 4) & 0xff;
	}
	/* if the address is invalid, use a random value */
	if (!is_valid_ether_addr(dev->dev_addr)) {
		struct sockaddr sa = { AF_UNSPEC };

		netdev_warn(dev, "Invalid MAC address, using random\n");
		eth_hw_addr_random(dev);
		memcpy(sa.sa_data, dev->dev_addr, ETH_ALEN);
		if (rtl838x_set_mac_address(dev, &sa))
			netdev_warn(dev, "Failed to set MAC address.\n");
	}
	pr_info("Using MAC %08x%08x\n", sw_r32(priv->r->mac),
					sw_r32(priv->r->mac + 4));
	strcpy(dev->name, "eth%d");
	priv->pdev = pdev;
	priv->netdev = dev;

	err = rtl838x_mdio_init(priv);
	if (err)
		goto err_free;

	err = register_netdev(dev);
	if (err)
		goto err_free;

	for (int i = 0; i < priv->rxrings; i++) {
		priv->rx_qs[i].id = i;
		priv->rx_qs[i].priv = priv;
		netif_napi_add(dev, &priv->rx_qs[i].napi, rtl838x_poll_rx, 64);
	}

	platform_set_drvdata(pdev, dev);

	phy_mode = PHY_INTERFACE_MODE_NA;
	err = of_get_phy_mode(dn, &phy_mode);
	if (err < 0) {
		dev_err(&pdev->dev, "incorrect phy-mode\n");
		err = -EINVAL;
		goto err_free;
	}
	priv->phylink_config.dev = &dev->dev;
	priv->phylink_config.type = PHYLINK_NETDEV;

	phylink = phylink_create(&priv->phylink_config, pdev->dev.fwnode,
				 phy_mode, &rtl838x_phylink_ops);

	if (IS_ERR(phylink)) {
		err = PTR_ERR(phylink);
		goto err_free;
	}
	priv->phylink = phylink;

	return 0;

err_free:
	pr_err("Error setting up netdev, freeing it again.\n");
	free_netdev(dev);

	return err;
}

static int rtl838x_eth_remove(struct platform_device *pdev)
{
	struct net_device *dev = platform_get_drvdata(pdev);
	struct rtl838x_eth_priv *priv = netdev_priv(dev);

	if (dev) {
		pr_info("Removing platform driver for rtl838x-eth\n");
		rtl838x_mdio_remove(priv);
		rtl838x_hw_stop(priv);

		netif_tx_stop_all_queues(dev);

		for (int i = 0; i < priv->rxrings; i++)
			netif_napi_del(&priv->rx_qs[i].napi);

		unregister_netdev(dev);
		free_netdev(dev);
	}

	return 0;
}

static const struct of_device_id rtl838x_eth_of_ids[] = {
	{ .compatible = "realtek,rtl838x-eth"},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, rtl838x_eth_of_ids);

static struct platform_driver rtl838x_eth_driver = {
	.probe = rtl838x_eth_probe,
	.remove = rtl838x_eth_remove,
	.driver = {
		.name = "rtl838x-eth",
		.pm = NULL,
		.of_match_table = rtl838x_eth_of_ids,
	},
};

module_platform_driver(rtl838x_eth_driver);

MODULE_AUTHOR("B. Koblitz");
MODULE_DESCRIPTION("RTL838X SoC Ethernet Driver");
MODULE_LICENSE("GPL");
