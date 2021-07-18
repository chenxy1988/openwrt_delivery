#
# Copyright (C) 2008-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=uart-zynq
PKG_RELEASE:=2
PKG_LICENSE:=GPL-2.0

include $(INCLUDE_DIR)/package.mk

define KernelPackage/uart-zynq
  SUBMENU:=Other modules
  TITLE:=Uart driver for zynq platform
  FILES:=$(PKG_BUILD_DIR)/uart-zynq.ko
  AUTOLOAD:=$(call AutoLoad,32,uart-zynq,1)
  KCONFIG:=
endef

define KernelPackage/uart-zynq/description
 This is a replacement for the following in-kernel drivers:
 uart-zynq
endef

MAKE_OPTS:= \
	$(KERNEL_MAKE_FLAGS) \
	SUBDIRS="$(PKG_BUILD_DIR)"

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		$(MAKE_OPTS) \
		modules
endef

$(eval $(call KernelPackage,uart-zynq))
