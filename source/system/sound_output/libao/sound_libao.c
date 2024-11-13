/*
 * libao output sound code.
 * License : MIT
 * See docs/MIT_license.txt for more information.
*/

#include <ao/ao.h>

#include "types.h"
#include "misc/log.h"
#include "misc/config.h"
#include "misc/memutil.h"
#include "system/sound.h"

static int sound_initialized = 0;

static ao_device *aodevice = NULL;
static ao_sample_format aoformat = { 0 };

int sound_init()
{
	ao_info *info;

	if(config_get_bool("sound.enabled") == 0)
		return(0);

	log_printf("sound_init: Start initialization\n");

	ao_initialize();
	
	aoformat.bits = 16;
	aoformat.channels = CHANNELS;
	aoformat.rate = SAMPLE_RATE;
	aoformat.byte_format = AO_FMT_LITTLE;
	
	aodevice = ao_open_live(ao_default_driver_id(), &aoformat, NULL); // Live output
	if (!aodevice)
		aodevice = ao_open_live(ao_driver_id("null"), &aoformat, NULL);
	
	info = ao_driver_info(ao_default_driver_id());
	return(0);
}

void sound_update(void *buffer,int length)
{
	ao_play(aodevice, (char*)buffer, length * 2);
}

void sound_kill()
{
	if (aodevice)
	{
		ao_close(aodevice);
		ao_shutdown();
	}
}

void sound_pause()
{
	sound_kill();
}

void sound_play()
{
	sound_init();
}

void sound_setfps(int fps)
{
}