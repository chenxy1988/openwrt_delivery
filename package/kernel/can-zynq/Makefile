#
# Copyright (C) 2008-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=can-zynq
PKG_RELEASE:=2
PKG_LICENSE:=GPL-2.0

include $(INCLUDE_DIR)/package.mk

define KernelPackage/can-zynq
  SUBMENU:=Other modules
  TITLE:=CAN driver for zynq platform
  FILES:=$(PKG_BUILD_DIR)/xilinx_can.ko
  AUTOLOAD:=$(call AutoLoad,34,xilinx_can,1)
  KCONFIG:=
endef

define KernelPackage/can-zynq/description
CAN xilinx
endef

MAKE_OPTS:= \
	$(KERNEL_MAKE_FLAGS) \
	SUBDIRS="$(PKG_BUILD_DIR)"

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		$(MAKE_OPTS) \
		modules
endef

$(eval $(call KernelPackage,can-zynq))
