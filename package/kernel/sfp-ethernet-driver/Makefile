#
# Copyright (C) 2008-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=sfp-ethernet-driver
PKG_RELEASE:=2
PKG_LICENSE:=GPL-2.0

include $(INCLUDE_DIR)/package.mk

define KernelPackage/sfp-ethernet-driver
  SUBMENU:=Other modules
  TITLE:=SFP ETHERNET BOTH PL AND PS SITE
  FILES:=$(PKG_BUILD_DIR)/sfp-ethernet.ko
  AUTOLOAD:=$(call AutoLoad,30,sfp-ethernet,1)
  KCONFIG:=
endef

define KernelPackage/sfp-ethernet-driver/description
 This is a replacement for the following in-kernel drivers:
 xilinx axiethernet driver
endef

MAKE_OPTS:= \
	$(KERNEL_MAKE_FLAGS) \
	SUBDIRS="$(PKG_BUILD_DIR)"

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		$(MAKE_OPTS) \
		modules
endef

$(eval $(call KernelPackage,sfp-ethernet-driver))
