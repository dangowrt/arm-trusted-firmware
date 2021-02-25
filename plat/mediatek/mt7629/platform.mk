#
# Copyright (c) 2020, MediaTek Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

MTK_PLAT		:=	plat/mediatek
MTK_PLAT_SOC		:=	$(MTK_PLAT)/$(PLAT)

# Not needed for Cortex-A7
WORKAROUND_CVE_2017_5715	:=	0

# indicate the reset vector address can be programmed
PROGRAMMABLE_RESET_ADDRESS	:=	1

# Do not enable SVE
ENABLE_SVE_FOR_NS	:=	0
MULTI_CONSOLE_API	:=	1

# AArch32-specific
AARCH32_SP		:=	sp_min
ARM_CORTEX_A7		:=	yes
ARM_ARCH_MAJOR		:=	7
MARCH32_DIRECTIVE	:=	-mcpu=cortex-a7 -msoft-float
RESET_TO_BL2		:=	1

PLAT_INCLUDES		:=	-I$(MTK_PLAT)/common				\
				-I$(MTK_PLAT)/include				\
				-I$(MTK_PLAT)/common/drivers/uart		\
				-I$(MTK_PLAT)/common/drivers/efuse		\
				-I$(MTK_PLAT)/common/drivers/efuse/include	\
				-Iinclude/plat/arm/common			\
				-I$(MTK_PLAT_SOC)/drivers/pll			\
				-I$(MTK_PLAT_SOC)/drivers/spm			\
				-I$(MTK_PLAT_SOC)/drivers/timer			\
				-I$(MTK_PLAT_SOC)/drivers/wdt			\
				-I$(MTK_PLAT_SOC)/include

PLAT_BL_COMMON_SOURCES	:=	lib/cpus/aarch32/cortex_a7.S			\
				$(MTK_PLAT)/common/drivers/uart/aarch32/hsuart.S\
				drivers/delay_timer/delay_timer.c		\
				drivers/delay_timer/generic_delay_timer.c	\
				$(MTK_PLAT)/common/mtk_plat_common.c		\
				$(MTK_PLAT_SOC)/aarch32/plat_helpers.S		\
				$(MTK_PLAT_SOC)/aarch32/platform_common.c

CPPFLAGS		+=	-D__SOFTFP__

include $(MTK_PLAT_SOC)/bl2/bl2.mk
include $(MTK_PLAT_SOC)/bl32.mk

include $(MTK_PLAT)/apsoc_common/bl2/tbbr_post.mk
include $(MTK_PLAT)/apsoc_common/bl2/bl2_image_post.mk
