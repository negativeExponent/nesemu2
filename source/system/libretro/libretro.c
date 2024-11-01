/***************************************************************************
 *   Copyright (C) 2013 by James Holodnak                                  *
 *   jamesholodnak@gmail.com                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <stdio.h>

#include <libretro.h>
#include <string/stdstring.h>
#include <file/file_path.h>
#include <streams/file_stream.h>
#include <streams/memory_stream.h>

#include "version.h"
#include "emu/emu.h"
#include "emu/commands.h"
#include "emu/events.h"
#include "misc/log.h"
#include "misc/config.h"
#include "misc/paths.h"
#include "misc/memutil.h"
#include "nes/nes.h"
#include "system/main.h"
#include "system/system.h"
#include "system/video.h"
#include "system/input.h"
#include "palette/palette.h"
#include "palette/generator.h"
#include "system/libretro/console/console.h"

//required
char configfilename[1024] = CONFIG_FILENAME;
char exepath[1024] = "";

static retro_video_refresh_t video_cb            = NULL;
static retro_input_poll_t input_poll_cb          = NULL;
static retro_audio_sample_t audio_cb             = NULL;
static retro_environment_t environ_cb            = NULL;
static struct retro_log_callback log_cb          = { 0 };

retro_input_state_t input_state_cb               = NULL;
retro_audio_sample_batch_t audio_batch_cb        = NULL;

bool libretro_supports_bitmasks = false;

static unsigned system_width = 256;
static unsigned system_height = 240;

static unsigned serialize_size = 0;

static void default_logger(enum retro_log_level level, const char *fmt, ...) {}
static void check_system_specs(void) {
	/* TODO - when we get it running at fullspeed on PSP, set to 4 */
	unsigned level = 5;
	environ_cb(RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL, &level);
}

RETRO_API void retro_set_environment(retro_environment_t cb) {
	static const struct retro_system_content_info_override content_overrides[] = {
		{
		    "fds|nes|unf|unif|nsf", /* extensions */
		    false,                       /* need_fullpath */
		    false                        /* persistent_data */
		},
		{ NULL, false, false }
	};
	environ_cb = cb;
	environ_cb(RETRO_ENVIRONMENT_SET_CONTENT_INFO_OVERRIDE, (void *)content_overrides);
}

RETRO_API void retro_set_video_refresh(retro_video_refresh_t cb) { video_cb = cb; }
RETRO_API void retro_set_audio_sample(retro_audio_sample_t cb) { audio_cb = cb; }
RETRO_API void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { audio_batch_cb = cb; }
RETRO_API void retro_set_input_poll(retro_input_poll_t cb) { input_poll_cb = cb; }
RETRO_API void retro_set_input_state(retro_input_state_t cb) { input_state_cb = cb; }

RETRO_API void retro_init(void) {
	log_cb.log = default_logger;
	environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log_cb);
	if (environ_cb(RETRO_ENVIRONMENT_GET_INPUT_BITMASKS, NULL))
		libretro_supports_bitmasks = true;
}

RETRO_API void retro_deinit(void) {
	emu_kill();
	emu_exit(0);
	libretro_supports_bitmasks = false;
}

RETRO_API unsigned retro_api_version(void) { return RETRO_API_VERSION; }

RETRO_API void retro_get_system_info(struct retro_system_info *info) {
	info->need_fullpath    = true;
	info->valid_extensions = "fds|nes|unf|unif|nsf";
#ifdef GIT_VERSION
	info->library_version = "0.6.1" GIT_VERSION;
#else
	info->library_version = "0.6.1";
#endif
	info->library_name  = "nesemu2";
	info->block_extract = false;
}

RETRO_API void retro_get_system_av_info(struct retro_system_av_info *info) {
	info->geometry.base_width = system_width;
	info->geometry.base_height = system_height;
	info->geometry.max_width = system_width;
	info->geometry.max_height = system_height;
	info->geometry.aspect_ratio = system_width * (8.0 / 7.0) / system_height;
	info->timing.fps = nes->region->fps;
	info->timing.sample_rate = 44100.0;
}

RETRO_API void retro_set_controller_port_device(unsigned port, unsigned device) {}

RETRO_API void retro_reset(void) {
	emu_event(E_SOFTRESET,0);
}

extern u32 *pixel32;

RETRO_API void retro_run(void) {
	static s16 sound_sample_buf[3 * 1024];
	s16 *sound;
	u32 ssize;
	int i;
	int width, height;

	input_poll_cb();

	width = video_getwidth();
	height = video_getheight();

	if ((system_width != width) || (system_height != height)) {
		struct retro_system_av_info av_info;
		system_width = width;
		system_height = height;
		retro_get_system_av_info(&av_info);
		environ_cb(RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO, &av_info);
	}

	system_checkevents();
	input_poll();
	video_startframe();
	if (running && nes->cart)
		nes_frame();
	video_endframe();

	video_cb(pixel32, system_width, system_height, system_width * sizeof(u32));
}

RETRO_API size_t retro_serialize_size(void) {
	if (serialize_size == 0) {
		memfile_t *file;
		size_t size = 1000000; /* something really big */
		u8 *buffer = (u8*)mem_alloc(size);

		file = memfile_open_memory(buffer, size);

		state_save(file);

		if (file->data) mem_free(file->data);
		serialize_size = memfile_tell(file);
		if (file) mem_free(file);
		if (buffer) mem_free(buffer);
	}
	return serialize_size;
}

RETRO_API bool retro_serialize(void *data, size_t size) {
	if (serialize_size == size) {
		memfile_t *file;
		u8 *_dat = (u8*)mem_alloc(size);

		if (!_dat) return false;

		file = memfile_open_memory(_dat, size);

		mem_free(_dat);

		state_save(file);

		memcpy(data, file->data, size);

		if (file->data) mem_free(file->data);
		mem_free(file);

		return true;
	}

	return false;
}

RETRO_API bool retro_unserialize(const void *data, size_t size) {
	if (serialize_size == size) {
		memfile_t *file;

		file = memfile_open_memory((u8*)data, size);

		state_load(file);

		if (file->data) mem_free(file->data);
		mem_free(file);

		return true;
	}

	return false;
}

RETRO_API void retro_cheat_reset(void) {}
RETRO_API void retro_cheat_set(unsigned index, bool enabled, const char *code) {}

RETRO_API bool retro_load_game(const struct retro_game_info *info) {
	struct retro_input_descriptor desc[] =
	{
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "D-Pad Left" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "D-Pad Up" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "D-Pad Down" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "B" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "A" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "D-Pad Left" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "D-Pad Up" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "D-Pad Down" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "B" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "A" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start" },
		{ 0 },
	};
	struct retro_game_info_ext *info_ext = NULL;
	const u8 *content_data               = NULL;
	size_t content_size                  = 0;
	char content_path[2048]              = { 0 };
	enum retro_pixel_format rgbformat    = RETRO_PIXEL_FORMAT_XRGB8888;

	environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);
	if (environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &rgbformat))
		log_cb.log(RETRO_LOG_INFO, " Frontend supports xRGB888 - will use that instead of XRGB1555.\n");

	/* Attempt to fetch extended game info */
	if (environ_cb(RETRO_ENVIRONMENT_GET_GAME_INFO_EXT, &info_ext) && info_ext) {
		content_data = (const u8 *)info_ext->data;
		content_size = info_ext->size;

		if (info_ext->file_in_archive) {
			/* We don't have a 'physical' file in this
			 * case, but the core still needs a filename
			 * in order to detect the region of iNES v1.0
			 * ROMs. We therefore fake it, using the content
			 * directory, canonical content name, and content
			 * file extension */
			snprintf(content_path, sizeof(content_path), "%s%c%s.%s", info_ext->dir, PATH_DEFAULT_SLASH_C(),
			    info_ext->name, info_ext->ext);
		} else {
			strlcpy(content_path, info_ext->full_path, sizeof(content_path));
		}
	} else {
		if (!info || string_is_empty(info->path)) {
			return false;
		}

		strlcpy(content_path, info->path, sizeof(content_path));
	}

	check_system_specs();
	
	//add extra subsystems
	emu_addsubsystem("console",console_init,console_kill);

	//initialize the emulator
	if(emu_init() != 0) {
        log_printf("main:  emu_init() failed\n");
        return(2);
	}

	//load rom specified by arguments
	if((nes_load((char*)content_path, (u8*)content_data, content_size)) == 0) {
		nes_reset(1);
		running = 1;
	}

	serialize_size = 0;

	return (nes->cart != NULL);
}

RETRO_API bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info) { return false; }
RETRO_API void retro_unload_game(void) {}
RETRO_API unsigned retro_get_region(void) { return 1; };
RETRO_API void *retro_get_memory_data(unsigned id) { return NULL; }
RETRO_API size_t retro_get_memory_size(unsigned id) { return 0; }
