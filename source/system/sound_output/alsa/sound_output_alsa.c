/*
 * Alsa output sound code.
 * License : MIT
 * See docs/MIT_license.txt for more information.
*/

#include <alsa/asoundlib.h>

#include "types.h"
#include "misc/log.h"
#include "misc/config.h"
#include "misc/memutil.h"
#include "system/sound.h"

static int initialized_audio = 0;

static snd_pcm_t *handle;

int sound_init(void) {
	snd_pcm_hw_params_t *params;
	u32 val;
	s32 dir = -1;
	snd_pcm_uframes_t frames;

	if(config_get_bool("sound.enabled") == 0)
		return(0);

	log_printf("sound_init: Start initialization\n");

	/* Open PCM device for playback. */
	s32 rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);

#ifdef ZIPIT_PORT
	if (rc < 0)
		rc = snd_pcm_open(&handle, "plughw:Z2,0,0", SND_PCM_STREAM_PLAYBACK, 0);
#endif

	if (rc < 0)
		rc = snd_pcm_open(&handle, "plughw:0,0,0", SND_PCM_STREAM_PLAYBACK, 0);

	if (rc < 0)
		rc = snd_pcm_open(&handle, "plughw:0,0", SND_PCM_STREAM_PLAYBACK, 0);

	if (rc < 0)
		rc = snd_pcm_open(&handle, "plughw:1,0,0", SND_PCM_STREAM_PLAYBACK, 0);

	if (rc < 0)
		rc = snd_pcm_open(&handle, "plughw:1,0", SND_PCM_STREAM_PLAYBACK, 0);

	if (rc < 0) {
		log_printf("unable to open PCM device: %s\n", snd_strerror(rc));
		return(1);
	}

	log_printf("sound_init: pcm name = %s\n", snd_pcm_name(handle));

#ifdef NONBLOCKING_AUDIO
	snd_pcm_nonblock(handle, 1);
#else
	snd_pcm_nonblock(handle, 0);
#endif

	/* Allocate a hardware parameters object. */
	/* snd_pcm_hw_params_alloca(&params); */
	rc = snd_pcm_hw_params_malloc(&params);
	if (rc < 0) {
		log_printf("Error:snd_pcm_hw_params_malloc %s\n", snd_strerror(rc));
		return(1);
	}

	/* Fill it in with default values. */
	rc = snd_pcm_hw_params_any(handle, params);
	if (rc < 0) {
		log_printf("Error:snd_pcm_hw_params_any %s\n", snd_strerror(rc));
		return(1);
	}

	/* Set the desired hardware parameters. */

	/* Interleaved mode */
	rc = snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
	if (rc < 0) {
		log_printf("Error:snd_pcm_hw_params_set_access %s\n", snd_strerror(rc));
		return(1);
	}

	/* Signed 16-bit little-endian format */
	rc = snd_pcm_hw_params_set_format(handle, params,
#ifdef MSB_FIRST
		SND_PCM_FORMAT_S16_BE
#else
		SND_PCM_FORMAT_S16_LE
#endif
	);
	if (rc < 0) {
		log_printf("Error:snd_pcm_hw_params_set_format %s\n", snd_strerror(rc));
		return(1);
	}

	/* Two channels (stereo) */
	rc = snd_pcm_hw_params_set_channels(handle, params, CHANNELS);
	if (rc < 0) {
		log_printf("Error:snd_pcm_hw_params_set_channels %s\n", snd_strerror(rc));
		return(1);
	}

	val = SAMPLE_RATE;
	rc = snd_pcm_hw_params_test_rate(handle, params, val, dir);
	if (rc < 0) {
		log_printf("Error:snd_pcm_hw_params_test_rate %s\n", snd_strerror(rc));
		return(1);
	}

	rc = snd_pcm_hw_params_set_rate_near(handle, params, &val, &dir);
	if (rc < 0) {
		log_printf("Error:snd_pcm_hw_params_set_rate_near %s\n", snd_strerror(rc));
		return(1);
	}

	/* Set period size to settings.aica.BufferSize frames. */
	frames = BUFFER_SIZE;
	rc = snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);
	if (rc < 0) {
		log_printf("Error:snd_pcm_hw_params_set_buffer_size_near %s\n", snd_strerror(rc));
		return(1);
	}
	frames *= 4;
	rc = snd_pcm_hw_params_set_buffer_size_near(handle, params, &frames);
	if (rc < 0) {
		log_printf("Error:snd_pcm_hw_params_set_buffer_size_near %s\n", snd_strerror(rc));
		return(1);
	}

	/* Write the parameters to the driver */
	rc = snd_pcm_hw_params(handle, params);
	if (rc < 0) {
		log_printf("Unable to set hw parameters: %s\n", snd_strerror(rc));
		return(1);
	}

	initialized_audio = 1;

	return(0);
}

void sound_update(void *buffer,int length) {
	if(initialized_audio) {
		long ret = snd_pcm_writei(handle, buffer, length);
		while(ret != length) {
			if (ret < 0) snd_pcm_prepare( handle );
			else length -= ret;
			ret = snd_pcm_writei(handle, buffer, length);
		}
	}
}

void sound_kill(void) {
	if (handle) {
		snd_pcm_drop(handle);
		snd_pcm_close(handle);
		snd_config_update_free_global();
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