# SPDX-License-Identifier: GPL-2.0-only

define Device/kernel
  KERNEL := \
    kernel-bin | \
    append-dtb | \
    lzma | \
    uImage lzma
endef

define Device/initramfs
  KERNEL_INITRAMFS := \
    kernel-bin | \
    append-dtb | \
    lzma | \
    uImage lzma
endef

define Device/zyxel_initramfs
  KERNEL_INITRAMFS := \
    kernel-bin | \
    append-dtb | \
    lzma | \
    zyxel-vers $$$$(ZYXEL_VERS) | \
    uImage lzma
endef

define Device/zyxel_xgs1200
  $(Device/kernel)
  $(Device/initramfs)
  DEVICE_VENDOR := Zyxel
  IMAGE_SIZE := 15424k
  SOC := rtl9302b
endef

define Device/zyxel_xgs1250-12
  SOC := rtl9302
  UIMAGE_MAGIC := 0x93001250
  ZYXEL_VERS := ABWE
  DEVICE_VENDOR := Zyxel
  DEVICE_MODEL := XGS1250-12
  IMAGE_SIZE := 13312k
  KERNEL := \
	kernel-bin | \
	append-dtb | \
	lzma | \
	uImage lzma
  KERNEL_INITRAMFS := \
	kernel-bin | \
	append-dtb | \
	lzma | \
	zyxel-vers $$$$(ZYXEL_VERS) | \
	uImage lzma
endef
TARGET_DEVICES += zyxel_xgs1250-12
