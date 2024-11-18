/*
 * Pulseaudio output sound code.
 * License : MIT
 * See docs/MIT_license.txt for more information.
*/

#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/unistd.h>
#include <pulse/simple.h>

#include "types.h"
#include "misc/log.h"
#include "misc/config.h"
#include "misc/memutil.h"
#include "system/sound.h"

static pa_simple *pulse_stream = NULL;

int sound_init()
{
	pa_sample_spec ss;
	pa_buffer_attr paattr;

	if(config_get_bool("sound.enabled") == 0)
		return(0);

	log_printf("sound_init: Start initialization\n");
	
	ss.format = PA_SAMPLE_S16LE;
	ss.channels = CHANNELS;
	ss.rate = SAMPLE_RATE;

	paattr.tlength = pa_bytes_per_second(&ss) / 5;
	paattr.prebuf = paattr.tlength;
	paattr.maxlength = paattr.tlength * 3;
	paattr.minreq = paattr.tlength / 3;

	/* Create a new playback stream */
	pulse_stream = pa_simple_new(NULL, "nesemu2", PA_STREAM_PLAYBACK, NULL, "SMS Plus GX", &ss, NULL, &paattr, NULL);
	if (!pulse_stream)
	{
		log_printf("PulseAudio: pa_simple_new() failed!\n");
	}

	return(0);
}

void sound_update(void *buffer,int length)
{
	if(pulse_stream != NULL) {
		if (pa_simple_write(pulse_stream, buffer, length * 2, NULL) < 0)
			log_printf("PulseAudio: pa_simple_write() failed!\n");
	}
}

void sound_kill()
{
	if(pulse_stream != NULL)
	{
		if (pa_simple_drain(pulse_stream, NULL) < 0) 
		{
			log_printf("PulseAudio: pa_simple_drain() failed!\n");
		}
		pa_simple_free(pulse_stream);
	}
}

void sound_pause()
{
}

void sound_play()
{
}

void sound_setfps(int fps)
{
}