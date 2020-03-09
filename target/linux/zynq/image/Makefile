#
# Copyright (C) 2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/image.mk

FAT32_BLOCK_SIZE=1024
FAT32_BLOCKS=$(shell echo $$(($(CONFIG_TARGET_KERNEL_PARTSIZE)*1024*1024/$(FAT32_BLOCK_SIZE))))

define Build/zynq-pmu-sdcard
        rm -f $(BIN_DIR)/boot.bin
        cp $(STAGING_DIR_IMAGE)/$(DEVICE_NAME)-u-boot.elf ./u-boot.elf
        cp $(TOPDIR)/target/linux/zynq/image/bootloader/$(DEVICE_IMGLAYOUT)/$(DEVICE_IMGLAYOUT).bif ./
        cp $(TOPDIR)/target/linux/zynq/image/bootloader/$(DEVICE_IMGLAYOUT)/fpga.bit ./
        cp $(TOPDIR)/target/linux/zynq/image/bootloader/$(DEVICE_IMGLAYOUT)/fsbl.elf ./
        $(TOPDIR)/staging_dir/host/bin/mkbootimage ./$(DEVICE_IMGLAYOUT).bif ./boot.bin
        cp ./boot.bin $(BIN_DIR)/boot.bin
        rm -f $(DEVICE_IMGLAYOUT).bif
        rm -f fpga.bit
        rm -f fsbl.elf
        rm -f boot.bin
        rm -f u-boot.elf
        rm -f $@.boot
        rm -f $@.bank1
        rm -f $@.bank2
        mkfs.fat $@.boot -C $(FAT32_BLOCKS) -n "boot"
        mcopy -i $@.boot $(BIN_DIR)/boot.bin ::boot.bin
        mcopy -i $@.boot $(STAGING_DIR_IMAGE)/$(DEVICE_NAME)-uEnv.txt ::uEnv.txt
        mcopy -i $@.boot $(KDIR)/xlnx_zynq-bms-fit-uImage.itb ::uImage0
        mcopy -i $@.boot $(KDIR)/image-$(DEVICE_DTS).dtb ::devicetree0.dtb
        ./gen_zynq_sdcard_dualbank_img.sh $@ \
                $@.boot \
                $(IMAGE_ROOTFS) \
                $(IMAGE_ROOTFS) \
                $(CONFIG_TARGET_KERNEL_PARTSIZE) \
                $(CONFIG_TARGET_ROOTFS_PARTSIZE)
	./gen_zynq_sdcard_upgrade.sh $@.upgrade \
		$(KDIR)/image-$(DEVICE_DTS).dtb \
		$(KDIR)/xlnx_zynq-bms-fit-uImage.itb \
		$(IMAGE_ROOTFS)
        mv $@.upgrade $(BIN_DIR)/
        rm -f $@.boot
        rm -f $@.bank1
        rm -f $@.bank2
endef

define Build/zynq-sdcard
	rm -f $@.boot
	mkfs.fat $@.boot -C $(FAT32_BLOCKS)
	mcopy -i $@.boot $(STAGING_DIR_IMAGE)/$(DEVICE_NAME)-boot.bin ::boot.bin
	mcopy -i $@.boot $(STAGING_DIR_IMAGE)/$(DEVICE_NAME)-u-boot.img ::u-boot.img
	mcopy -i $@.boot $(STAGING_DIR_IMAGE)/$(DEVICE_NAME)-uEnv.txt ::uEnv.txt
	mcopy -i $@.boot $(IMAGE_KERNEL) ::fit.itb
	./gen_zynq_sdcard_img.sh $@ \
		$@.boot \
		$(IMAGE_ROOTFS) \
		$(CONFIG_TARGET_KERNEL_PARTSIZE) \
		$(CONFIG_TARGET_ROOTFS_PARTSIZE)
	rm -f $@.boot
endef

#################################################
# Default and templates
#################################################

define Device/Default
	PROFILES := Default
	KERNEL_DEPENDS = $$(wildcard $(DTS_DIR)/$$(DEVICE_DTS).dts)
	KERNEL_INITRAMFS_PREFIX := $$(IMG_PREFIX)-$(1)-initramfs
	KERNEL_PREFIX := $$(IMAGE_PREFIX)
	KERNEL_LOADADDR := 0x8000
	IMAGES := sdcard.img
	IMAGE/sdcard.img := zynq-pmu-sdcard
endef

define Device/FitImageGzip
	DEVICE_DTS_DIR := ../dts/$$(DEVICE_DTS)
	KERNEL_SUFFIX := -fit-uImage.itb
	KERNEL = kernel-bin | gzip | fit gzip $$(DTS_DIR)/$$(DEVICE_DTS).dtb
	KERNEL_NAME := Image
endef

#################################################
# Devices
#################################################
define Device/xlnx_zynq-bms
        DEVICE_DTS := zynq-bms7015
        $(call Device/FitImageGzip)
        DEVICE_TITLE := ZYNQ_7015_BMS
        DEVICE_IMGLAYOUT := zynq-bms7015
        DEVICE_PACKAGES:=kmod-can kmod-can-xilinx-can
endef
TARGET_DEVICES += xlnx_zynq-bms

$(eval $(call BuildImage))
