#
# Copyright (C) 2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_VERSION:=2019.01
PKG_NAME:=u-boot
PKG_VERSION:=2019.01
PKG_SOURCE_URL:=/home/chenxy/zynq-project/u-boot-xlnx
PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=2fad6970f5ea5cbed23e73f442aa37690fbd87bd
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz


include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/u-boot.mk
include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/host-build.mk

define U-Boot/Default
  BUILD_TARGET:=zynq
  UBOOT_IMAGE:=spl/boot.bin u-boot.img
  UBOOT_CONFIG:=zynq_$(1)
  UENV:=default
#  HIDDEN:=1
  CATEGORY:=Boot loaders
endef

define U-Boot/zc702
  NAME:=Xilinx ZC702 Dev Board
  BUILD_DEVICES:=xlnx_zynq-zc702
endef

define U-Boot/bms7015
  NAME:=Xilinx ZC7015 BMS project
  BUILD_DEVICE:=xlnx_zynq-bms
endef

define U-Boot/zed
  NAME:=Avnet Digilent ZedBoard Dev Board
  BUILD_DEVICES:=avnet_zynq-zed
endef

define U-Boot/zybo
  NAME:=Digilent Zybo Dev Board
  BUILD_DEVICES:=digilent_zynq-zybo
endef

define U-Boot/zybo_z7
  NAME:=Digilent Zybo Z7 board
  BUILD_DEVICES:=digilent_zynq-zybo-z7
endef

UBOOT_TARGETS := \
	zc702 \
	zed \
	zybo \
	zybo_z7 \
	bms7015

define Build/InstallDev
	$(INSTALL_DIR) $(STAGING_DIR_IMAGE)
	$(CP) $(PKG_BUILD_DIR)/spl/boot.bin $(STAGING_DIR_IMAGE)/$(BUILD_DEVICE)-boot.bin
	$(CP) $(PKG_BUILD_DIR)/u-boot.img $(STAGING_DIR_IMAGE)/$(BUILD_DEVICE)-u-boot.img
	$(CP) $(PKG_BUILD_DIR)/u-boot $(STAGING_DIR_IMAGE)/$(BUILD_DEVICE)-u-boot.elf
	$(CP) ./files/uEnv-$(UENV).txt $(STAGING_DIR_IMAGE)/$(BUILD_DEVICE)-uEnv.txt
endef

define Package/u-boot/install/default
endef

Build/Exports:=$(Host/Exports)

$(eval $(call BuildPackage/U-Boot))
