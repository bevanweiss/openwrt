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

define Device/zyxel_xgs1010-12
  $(Device/zyxel_xgs1200)
  UIMAGE_MAGIC := 0x93001010
  DEVICE_MODEL := XGS1010-12
endef
TARGET_DEVICES += zyxel_xgs1010-12

define Device/zyxel_xgs1010-12_vendor
  $(Device/zyxel_xgs1010-12)
  $(Device/zyxel_initramfs)
  DEVICE_MODEL := XGS1010-12_vendor
  IMAGE_SIZE := 15232k
  ZYXEL_VERS := ABTY
endef
TARGET_DEVICES += zyxel_xgs1010-12_vendor

define Device/zyxel_xgs1210-12
  $(Device/zyxel_xgs1200)
  DEVICE_MODEL := XGS1210-12
  UIMAGE_MAGIC := 0x93001210
endef
TARGET_DEVICES += zyxel_xgs1210-12

define Device/zyxel_xgs1210-12_vendor
  $(Device/zyxel_xgs1210-12)
  $(Device/zyxel_initramfs)
  DEVICE_MODEL := XGS1210-12_vendor
  IMAGE_SIZE := 13184k
  ZYXEL_VERS := ABTY
endef
TARGET_DEVICES += zyxel_xgs1210-12_vendor

define Device/zyxel_xgs1250-12
  $(Device/zyxel_xgs1200)
  DEVICE_MODEL := XGS1250-12
  UIMAGE_MAGIC := 0x93001250
endef
TARGET_DEVICES += zyxel_xgs1250-12

define Device/zyxel_xgs1250-12_vendor
  $(Device/zyxel_xgs1250-12)
  $(Device/zyxel_initramfs)
  DEVICE_MODEL := XGS1250-12_vendor
  IMAGE_SIZE := 13184k
  ZYXEL_VERS := ABWE
endef
TARGET_DEVICES += zyxel_xgs1250-12_vendor
