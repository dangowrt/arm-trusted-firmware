#
# Copyright (c) 2022, MediaTek Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

MTK_PLAT		:=	plat/mediatek
MTK_PLAT_SOC		:=	$(MTK_PLAT)/$(PLAT)

# Enable workarounds for selected Cortex-A73 erratas.
ERRATA_A73_852427	:=	1
ERRATA_A73_855423	:=	1

# indicate the reset vector address can be programmed
PROGRAMMABLE_RESET_ADDRESS	:=	1

# Do not enable SVE
ENABLE_SVE_FOR_NS	:=	0
MULTI_CONSOLE_API	:=	1

RESET_TO_BL2		:=	1

PLAT_INCLUDES		:=	-Iinclude/plat/arm/common			\
				-Iinclude/plat/arm/common/aarch64		\
				-I$(MTK_PLAT)/common				\
				-I$(MTK_PLAT)/include				\
				-I$(MTK_PLAT)/common/drivers/uart		\
				-I$(MTK_PLAT)/common/drivers/gpt		\
				-I$(MTK_PLAT)/common/drivers/efuse		\
				-I$(MTK_PLAT)/common/drivers/efuse/include	\
				-I$(MTK_PLAT_SOC)/drivers/efuse/include		\
				-I$(MTK_PLAT_SOC)/drivers/dram			\
				-I$(MTK_PLAT_SOC)/drivers/pll			\
				-I$(MTK_PLAT_SOC)/drivers/spmc			\
				-I$(MTK_PLAT_SOC)/drivers/timer			\
				-I$(MTK_PLAT_SOC)/drivers/trng			\
				-I$(MTK_PLAT_SOC)/drivers/devapc		\
				-I$(MTK_PLAT_SOC)/drivers/wdt			\
				-I$(MTK_PLAT_SOC)/include

include $(MTK_PLAT_SOC)/bl2pl/bl2pl.mk
include $(MTK_PLAT_SOC)/bl2/bl2.mk
include $(MTK_PLAT_SOC)/bl31.mk
include $(MTK_PLAT_SOC)/drivers/efuse/efuse.mk

include $(MTK_PLAT)/apsoc_common/bl2/tbbr_post.mk
include $(MTK_PLAT)/apsoc_common/bl2/ar_post.mk
include $(MTK_PLAT)/apsoc_common/bl2/bl2_image_post.mk

OPTEE_TZRAM_SIZE := 0x10000
ifneq ($(BL32),)
ifeq ($(TRUSTED_BOARD_BOOT),1)
CPPFLAGS += -DNEED_BL32
OPTEE_TZRAM_SIZE := 0xfb0000
endif
endif
CPPFLAGS += -DOPTEE_TZRAM_SIZE=$(OPTEE_TZRAM_SIZE)

# Make sure make command parameter reflects on .o files immediately
include make_helpers/dep.mk

$(call GEN_DEP_RULES,bl2,emicfg bl2_boot_ram bl2_boot_nand_nmbm bl2_dev_mmc mtk_efuse bl2_plat_init bl2_plat_setup mt7988_gpio pll)
$(call MAKE_DEP,bl2,emicfg,DRAM_USE_COMB DRAM_USE_DDR4 DRAM_SIZE_LIMIT DRAM_DEBUG_LOG)
$(call MAKE_DEP,bl2,bl2_plat_init,BL2_COMPRESS I2C_SUPPORT EIP197_SUPPORT BL2_CPU_FULL_SPEED)
$(call MAKE_DEP,bl2,bl2_plat_setup,BOOT_DEVICE TRUSTED_BOARD_BOOT)
$(call MAKE_DEP,bl2,bl2_dev_mmc,BOOT_DEVICE)
$(call MAKE_DEP,bl2,bl2_boot_ram,RAM_BOOT_DEBUGGER_HOOK RAM_BOOT_UART_DL)
$(call MAKE_DEP,bl2,bl2_boot_nand_nmbm,NMBM_MAX_RATIO NMBM_MAX_RESERVED_BLOCKS NMBM_DEFAULT_LOG_LEVEL)
$(call MAKE_DEP,bl2,mtk_efuse,ANTI_ROLLBACK TRUSTED_BOARD_BOOT)
$(call MAKE_DEP,bl2,mt7988_gpio,ENABLE_JTAG)
$(call MAKE_DEP,bl2,pll,BL2_CPU_FULL_SPEED)

$(call GEN_DEP_RULES,bl31,mtk_efuse plat_sip_calls)
$(call MAKE_DEP,bl31,mtk_efuse,ANTI_ROLLBACK TRUSTED_BOARD_BOOT)
$(call MAKE_DEP,bl31,plat_sip_calls,FWDL)
