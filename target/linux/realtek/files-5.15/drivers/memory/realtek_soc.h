
#ifndef __REALTEK_SOC_MEMORY_H
#define __REALTEK_SOC_MEMORY_H __FILE__

#define RTL_MCR_REG                             (0x1000)
#define RTL_MCR_DRAMTYPE                                GENMASK(31, 28) /* RO */
#define RTL_MCR_DRAMTYPE_DDR(val) \
        ((val) + 1)
#define RTL_MCR_BOOTSEL                                 GENMASK(27, 24) /* RO */
#define RTL_MCR_BOOTSEL_SPI_NOR                                 0x0
#define RTL_MCR_BOOTSEL_SPI_NAND_ROM                            0x1
#define RTL_MCR_BOOTSEL_PARALLEL_NAND_AUTO_LOAD                 0x2
#define RTL_MCR_BOOTSEL_PARALLEL_NAND_ROM                       0x3
#define RTL_MCR_BOOTSEL_SPI_NOR_ROM_CRYPT                       0x4
#define RTL_MCR_BOOTSEL_SPI_NAND_ROM_CRYPT                      0x5
#define RTL_MCR_IPREF                                   BIT(23)
#define RTL_MCR_DPREF                                   BIT(22)
#define RTL_MCR_EEPROM_TYPE                             BIT(21)         /* RO */
#define RTL_MCR_EEPROM_TYPE_8BIT_ADDR                           0b0
#define RTL_MCR_EEPROM_TYPE_16BIT_ADDR                          0b1
#define RTL_MCR_DRAM_SIGNAL_DISABLE                     BIT(20)         /* RO */
#define RTL_MCR_FLASH_MAP0_DISABLE                      BIT(19)         /* RW */
#define RTL_MCR_FLASH_MAP1_DISABLE                      BIT(18)
/* Reserved                                             17 - 15 */
#define RTL_MCR_DRAM_INIT_TRIGGER                       BIT(14)
#define RTL_MCR_DRAM_LX0_FREQ_STATUS                    BIT(13)
#define RTL_MCR_DRAM_LX0_FREQ_STATUS_FASTER                     0b0
#define RTL_MCR_DRAM_LX0_FREQ_STATUS_SLOWER                     0b1
#define RTL_MCR_DRAM_LX1_FREQ_STATUS                    BIT(12)
#define RTL_MCR_DRAM_LX1_FREQ_STATUS_FASTER                     0b0
#define RTL_MCR_DRAM_LX1_FREQ_STATUS_SLOWER                     0b1
#define RTL_MCR_DRAM_LX2_FREQ_STATUS                    BIT(11)
#define RTL_MCR_DRAM_LX2_FREQ_STATUS_FASTER                     0b0
#define RTL_MCR_DRAM_LX2_FREQ_STATUS_SLOWER                     0b1
#define RTL_MCR_DRAM_LX3_FREQ_STATUS                    BIT(10)
#define RTL_MCR_DRAM_LX3_FREQ_STATUS_FASTER                     0b0
#define RTL_MCR_DRAM_LX3_FREQ_STATUS_SLOWER                     0b1
#define RTL_MCR_DRAM_OCP0_FREQ_STATUS                   BIT(9)
#define RTL_MCR_DRAM_OCP0_FREQ_STATUS_FASTER                    0b0
#define RTL_MCR_DRAM_OCP0_FREQ_STATUS_SLOWER                    0b1
/* Reserved                                             8 - 0 */
/* Registers marked read-only/read-write bootstrap pins */

#define RTL_DCR_REG                             (0x1004)
/* Reserved                                             31 - 30 */
#define RTL_DCR_BANKCNT                                 GENMASK(29, 28)
#define RTL_DCR_BANKCNT_BANKS(val) \
        (1 << ((val) + 1))
/* Reserved                                             27 - 26 */
#define RTL_DCR_BUS_WIDTH                               GENMASK(25, 24)
#define RTL_DCR_BUS_WIDTH_BIT(val) \
        (1 << ((val) + 3))
#define RTL_DCR_ROWCNT                                  GENMASK(23, 20)
#define RTL_DCR_ROWCNT_ROWS(val) \
        (1 << ((val) + 11))
#define RTL_DCR_COLCNT                                  GENMASK(19, 16)
#define RTL_DCR_COLCNT_COLS(val) \
        (1 << ((val) + 8))
#define RTL_DCR_CS1_EN                                  BIT(15)
#define RTL_DCR_FAST_RX                                 BIT(14)
#define RTL_DCR_BURST_REFRESH                           BIT(13)
#define RTL_DCR_PARALLEL_BANK_ACTIVATION_EN             BIT(12)
/* Reserved                                             11 - 0 */

#define RTL_MC_DTR0_REG                         (0x1008)
#define RTL_MC_DTR1_REG                         (0x100c)
#define RTL_MC_DTR2_REG                         (0x1010)
#define RTL_MC_DMCR_REG                         (0x101c)
#define RTL_MC_DACCR_REG                        (0x1500)
#define RTL_MC_DCDR_REG                         (0x1060)

#endif /* __REALTEK_SOC_MEMORY_H */
