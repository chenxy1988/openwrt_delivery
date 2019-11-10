#
# Copyright (C) 2008-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=spi-zynq-userspace
PKG_RELEASE:=2
PKG_LICENSE:=GPL-2.0

include $(INCLUDE_DIR)/package.mk

define KernelPackage/spi-zynq-userspace
  SUBMENU:=Other modules
  TITLE:=SPI driver for zynq platform
  FILES:=$(PKG_BUILD_DIR)/spi-cadence.ko
  AUTOLOAD:=$(call AutoLoad,31,spi-cadence,1)
  KCONFIG:=
endef

define KernelPackage/spi-zynq-userspace/description
 This is a replacement for the following in-kernel drivers:
 spi-canence
endef

MAKE_OPTS:= \
	$(KERNEL_MAKE_FLAGS) \
	SUBDIRS="$(PKG_BUILD_DIR)"

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		$(MAKE_OPTS) \
		modules
endef

$(eval $(call KernelPackage,spi-zynq-userspace))
