/*
 * Copyright (c) 2013-2015, ARM Limited and Contributors. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * Neither the name of ARM nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <arch.h>
#include <arch_helpers.h>
#include <assert.h>
#include <auth_mod.h>
#include <bl_common.h>
#include <debug.h>
#include <platform.h>
#include <platform_def.h>
#include "bl1_private.h"


static void bl1_load_bl2(void);

/*******************************************************************************
 * The next function has a weak definition. Platform specific code can override
 * it if it wishes to.
 ******************************************************************************/
#pragma weak bl1_init_bl2_mem_layout

/*******************************************************************************
 * Function that takes a memory layout into which BL2 has been loaded and
 * populates a new memory layout for BL2 that ensures that BL1's data sections
 * resident in secure RAM are not visible to BL2.
 ******************************************************************************/
void bl1_init_bl2_mem_layout(const meminfo_t *bl1_mem_layout,
			     meminfo_t *bl2_mem_layout)
{
	const size_t bl1_size = BL1_RAM_LIMIT - BL1_RAM_BASE;

	assert(bl1_mem_layout != NULL);
	assert(bl2_mem_layout != NULL);

	/* Check that BL1's memory is lying outside of the free memory */
	assert((BL1_RAM_LIMIT <= bl1_mem_layout->free_base) ||
	       (BL1_RAM_BASE >= bl1_mem_layout->free_base +
				bl1_mem_layout->free_size));

	/* Remove BL1 RW data from the scope of memory visible to BL2 */
	*bl2_mem_layout = *bl1_mem_layout;
	reserve_mem(&bl2_mem_layout->total_base,
		    &bl2_mem_layout->total_size,
		    BL1_RAM_BASE,
		    bl1_size);

	flush_dcache_range((unsigned long)bl2_mem_layout, sizeof(meminfo_t));
}

/*******************************************************************************
 * Function to perform late architectural and platform specific initialization.
 * It also queries the platform to load and run next BL image. Only called
 * by the primary cpu after a cold boot.
 ******************************************************************************/
void bl1_main(void)
{
	unsigned int image_id;

	/* Announce our arrival */
	NOTICE(FIRMWARE_WELCOME_STR);
	NOTICE("BL1: %s\n", version_string);
	NOTICE("BL1: %s\n", build_message);

	INFO("BL1: RAM 0x%lx - 0x%lx\n", BL1_RAM_BASE, BL1_RAM_LIMIT);


#if DEBUG
	unsigned long val;
	/*
	 * Ensure that MMU/Caches and coherency are turned on
	 */
	val = read_sctlr_el3();
	assert(val & SCTLR_M_BIT);
	assert(val & SCTLR_C_BIT);
	assert(val & SCTLR_I_BIT);
	/*
	 * Check that Cache Writeback Granule (CWG) in CTR_EL0 matches the
	 * provided platform value
	 */
	val = (read_ctr_el0() >> CTR_CWG_SHIFT) & CTR_CWG_MASK;
	/*
	 * If CWG is zero, then no CWG information is available but we can
	 * at least check the platform value is less than the architectural
	 * maximum.
	 */
	if (val != 0)
		assert(CACHE_WRITEBACK_GRANULE == SIZE_FROM_LOG2_WORDS(val));
	else
		assert(CACHE_WRITEBACK_GRANULE <= MAX_CACHE_LINE_SIZE);
#endif

	/* Perform remaining generic architectural setup from EL3 */
	bl1_arch_setup();

#if TRUSTED_BOARD_BOOT
	/* Initialize authentication module */
	auth_mod_init();
#endif /* TRUSTED_BOARD_BOOT */

	/* Perform platform setup in BL1. */
	bl1_platform_setup();

	/* Get the image id of next image to load and run. */
	image_id = bl1_plat_get_next_image_id();

	if (image_id == BL2_IMAGE_ID)
		bl1_load_bl2();

	bl1_prepare_next_image(image_id);
}

/*******************************************************************************
 * This function locates and loads the BL2 raw binary image in the trusted SRAM.
 * Called by the primary cpu after a cold boot.
 * TODO: Add support for alternative image load mechanism e.g using virtio/elf
 * loader etc.
 ******************************************************************************/
void bl1_load_bl2(void)
{
	image_desc_t *image_desc;
	image_info_t *image_info;
	entry_point_info_t *ep_info;
	meminfo_t *bl1_tzram_layout;
	meminfo_t *bl2_tzram_layout;
	int err;

	/* Get the image descriptor */
	image_desc = bl1_plat_get_image_desc(BL2_IMAGE_ID);
	assert(image_desc);

	/* Get the image info */
	image_info = &image_desc->image_info;

	/* Get the entry point info */
	ep_info = &image_desc->ep_info;

	/* Find out how much free trusted ram remains after BL1 load */
	bl1_tzram_layout = bl1_plat_sec_mem_layout();

	INFO("BL1: Loading BL2\n");

	/* Load the BL2 image */
	err = load_auth_image(bl1_tzram_layout,
			 BL2_IMAGE_ID,
			 image_info->image_base,
			 image_info,
			 ep_info);

	if (err) {
		ERROR("Failed to load BL2 firmware.\n");
		plat_error_handler(err);
	}

	/*
	 * Create a new layout of memory for BL2 as seen by BL1 i.e.
	 * tell it the amount of total and free memory available.
	 * This layout is created at the first free address visible
	 * to BL2. BL2 will read the memory layout before using its
	 * memory for other purposes.
	 */
	bl2_tzram_layout = (meminfo_t *) bl1_tzram_layout->free_base;
	bl1_init_bl2_mem_layout(bl1_tzram_layout, bl2_tzram_layout);

	ep_info->args.arg1 = (unsigned long)bl2_tzram_layout;
	NOTICE("BL1: Booting BL2\n");
	VERBOSE("BL1: BL2 memory layout address = 0x%llx\n",
		(unsigned long long) bl2_tzram_layout);
}

/*******************************************************************************
 * Function called just before handing over to BL31 to inform the user about
 * the boot progress. In debug mode, also print details about the BL31 image's
 * execution context.
 ******************************************************************************/
void bl1_print_bl31_ep_info(const entry_point_info_t *bl31_ep_info)
{
	NOTICE("BL1: Booting BL3-1\n");
	print_entry_point_info(bl31_ep_info);
}

#if SPIN_ON_BL1_EXIT
void print_debug_loop_message(void)
{
	NOTICE("BL1: Debug loop, spinning forever\n");
	NOTICE("BL1: Please connect the debugger to continue\n");
}
#endif
