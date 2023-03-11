# SPDX-License-Identifier: GPL-2.0-only
define Device/hasivo_s1100wp-8gt-se
  SOC := rtl9303
  DEVICE_VENDOR := Hasivo
  DEVICE_MODEL := S1100WP-8GT-SE
  IMAGE_SIZE := 8364k
  KERNEL_INITRAMFS := \
	kernel-bin | \
	append-dtb | \
	gzip | \
	uImage gzip
endef
TARGET_DEVICES += hasivo_s1100wp-8gt-se

define Device/zyxel_xgs1250-12
  SOC := rtl9302
  UIMAGE_MAGIC := 0x93001250
  ZYXEL_VERS := ABWE
  DEVICE_VENDOR := Zyxel
  DEVICE_MODEL := XGS1250-12
  IMAGE_SIZE := 13312k
  KERNEL_INITRAMFS := \
	kernel-bin | \
	append-dtb | \
	gzip | \
	zyxel-vers $$$$(ZYXEL_VERS) | \
	uImage gzip
endef
TARGET_DEVICES += zyxel_xgs1250-12
