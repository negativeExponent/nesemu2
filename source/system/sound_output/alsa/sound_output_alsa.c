/*
 * ALSA Output Sound Code
 * License: MIT
 * See docs/MIT_license.txt for more details.
 */

#include <alsa/asoundlib.h>
#include "types.h"
#include "misc/log.h"
#include "misc/config.h"
#include "misc/memutil.h"
#include "system/sound.h"

/* Define global variables */
static int initialized_audio = 0;
static snd_pcm_t *handle = NULL;
static snd_pcm_uframes_t period_size, buffer_size;
static u8 *silent_period = NULL;

#define BITS_PER_SAMPLE 16
#define SAMPLE_RATE 44100  /* 44.1 kHz sample rate */
#define LATENCY_MS 40      /* Desired latency in milliseconds */
#define MIN_PERIOD_SIZE 64    /* Minimum period size in frames (safe limit) */
#define MAX_PERIOD_SIZE 8192  /* Maximum period size in frames (safe limit) */
#define MIN_BUFFER_SIZE 128   /* Minimum buffer size in frames */
#define MAX_BUFFER_SIZE 16384 /* Maximum buffer size in frames */

/**
 * Calculates period size based on latency requirements.
 * The period size is adjusted to be a power of 2, and is capped within a valid range.
 * 
 * @param latency_ms The latency requirement in milliseconds.
 * @param sample_rate The sample rate in Hz.
 * @param min_period_size The minimum allowed period size.
 * @param max_period_size The maximum allowed period size.
 * @return The calculated period size in frames.
 */
static snd_pcm_uframes_t calculate_period_size(int latency_ms, int sample_rate, snd_pcm_uframes_t min_period_size, snd_pcm_uframes_t max_period_size) {
	int samples = sample_rate * latency_ms / 1000; /* samples for desired latency */
	snd_pcm_uframes_t period_size = 1;

	/* Find the nearest power of 2 that fits the calculated sample count */
	while (period_size < samples) {
		period_size <<= 1; /* Double the period size until it exceeds the sample count */
	}

	/* Ensure the period size is within the allowed range */
	if (period_size < min_period_size) {
		period_size = min_period_size; /* Enforce minimum limit */
	}
	if (period_size > max_period_size) {
		period_size = max_period_size; /* Enforce maximum limit */
	}

	return period_size;
}

/**
 * Calculates buffer size based on the period size.
 * The buffer size should be a multiple of the period size and within valid limits.
 * 
 * @param period_size The calculated period size in frames.
 * @param min_buffer_size The minimum allowed buffer size.
 * @param max_buffer_size The maximum allowed buffer size.
 * @return The calculated buffer size in frames.
 */
static snd_pcm_uframes_t calculate_buffer_size(snd_pcm_uframes_t period_size, snd_pcm_uframes_t min_buffer_size, snd_pcm_uframes_t max_buffer_size) {
	snd_pcm_uframes_t buffer_size = 2 * period_size; /* Default: set buffer size to 2x period size */

	/* Ensure the buffer size is within valid limits */
	if (buffer_size < min_buffer_size) {
		buffer_size = min_buffer_size; /* Enforce minimum buffer size */
	}
	if (buffer_size > max_buffer_size) {
		buffer_size = max_buffer_size; /* Enforce maximum buffer size */
	}

	/* Ensure the buffer size is a multiple of the period size */
	if (buffer_size % period_size != 0) {
		buffer_size = (buffer_size / period_size) * period_size; /* Round down to nearest multiple of period_size */
	}

	return buffer_size;
}

/**
 * Initializes the sound system and configures ALSA playback.
 */
int sound_init(void) {
	snd_pcm_hw_params_t *params;
	u32 rate = SAMPLE_RATE;  /* Set the sample rate to 44.1kHz */
	int rc;

	/* If sound is disabled in configuration, skip initialization */
	if (config_get_bool("sound.enabled") == 0) {
		return 0;
	}

	log_printf("sound_init: Initializing ALSA sound system...\n");

	/* Open PCM device for playback (attempt various hardware devices if "default" fails) */
	rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
	if (rc < 0) {
		const char *device_names[] = {
			"plughw:Z2,0,0", "plughw:0,0,0", "plughw:0,0", "plughw:1,0,0", "plughw:1,0"
		};
		int i;
		for (i = 0; i < sizeof(device_names) / sizeof(device_names[0]); ++i) {
			rc = snd_pcm_open(&handle, device_names[i], SND_PCM_STREAM_PLAYBACK, 0);
			if (rc >= 0) break; /* Device successfully opened */
		}
	}

	if (rc < 0) {
		log_printf("Unable to open PCM device: %s\n", snd_strerror(rc));
		return 1; /* Return error if device could not be opened */
	}

	/* Set non-blocking mode for audio output (depends on compilation flags) */
#ifdef NONBLOCKING_AUDIO
	snd_pcm_nonblock(handle, 1);
#else
	snd_pcm_nonblock(handle, 0);
#endif

	log_printf("sound_init: PCM device name = %s\n", snd_pcm_name(handle));

	/* Allocate memory for ALSA hardware parameters */
	snd_pcm_hw_params_malloc(&params);

	/* Set ALSA hardware parameters to default values */
	rc = snd_pcm_hw_params_any(handle, params);
	if (rc < 0) {
		log_printf("sound_init: Failed to initialize hardware params: %s\n", snd_strerror(rc));
		return 1;
	}

	/* Calculate period size based on latency and the sample rate */
	period_size = calculate_period_size(LATENCY_MS, SAMPLE_RATE, MIN_PERIOD_SIZE, MAX_PERIOD_SIZE);
	buffer_size = calculate_buffer_size(period_size, MIN_BUFFER_SIZE, MAX_BUFFER_SIZE); /* Calculate buffer size */

	/* Set the desired audio parameters (interleaved, 16-bit, mono) */
	rc = snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
	rc |= snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);  /* 16-bit signed little-endian */
	rc |= snd_pcm_hw_params_set_channels(handle, params, 1); /* Mono (1 channel) */
	rc |= snd_pcm_hw_params_set_rate_near(handle, params, &rate, 0);
	rc |= snd_pcm_hw_params_set_period_size_near(handle, params, &period_size, NULL);
	rc |= snd_pcm_hw_params_set_buffer_size_near(handle, params, &buffer_size);

	if (rc < 0) {
		log_printf("sound_init: Failed to set ALSA params: %s\n", snd_strerror(rc));
		return 1;
	}

	/* Apply the configured parameters to the PCM device */
	rc = snd_pcm_hw_params(handle, params);
	if (rc < 0) {
		log_printf("sound_init: Failed to apply ALSA params: %s\n", snd_strerror(rc));
		return 1;
	}

	/* Log ALSA configuration information */
	log_printf("ALSA Initialized Successfully with the following parameters:\n");
	log_printf("  Sample Rate: %u Hz\n", rate);
	log_printf("  Channels: %u\n", 1);  /* Mono */
	log_printf("  Buffer Size: %lu frames\n", buffer_size);
	log_printf("  Period Size: %lu frames\n", period_size);
	log_printf("  Bytes per Sample: %d\n", BITS_PER_SAMPLE / 8);
	log_printf("  Estimated Latency: %.2f ms\n", (double)period_size / SAMPLE_RATE * 1000.0);

	/* Prepare PCM device for playback */
	rc = snd_pcm_prepare(handle);
	if (rc != 0) {
		log_printf("sound_init: snd_pcm_prepare failed: %s\n", snd_strerror(rc));
		return 1;
	}

	/* Start playback */
	rc = snd_pcm_start(handle);
	if (rc != 0) {
		log_printf("sound_init: snd_pcm_start failed: %s\n", snd_strerror(rc));
		return 1;
	}

	silent_period = mem_realloc(silent_period, period_size * CHANNELS * sizeof(s16));

	initialized_audio = 1; /* Mark audio as initialized */
	return 0; /* Success */
}

/**
 * Writes audio data to the PCM device.
 * 
 * @param buffer The audio data to be written.
 * @param length The length of the buffer in frames.
 */
void sound_update(void *buffer, int length) {
	long ret;

	if (initialized_audio) {
		ret = snd_pcm_writei(handle, buffer, length);
		if (ret < 0) {
			ret = snd_pcm_recover(handle, ret, 1); /* Attempt to recover from error */
			if (ret != 0) {
                log_printf("sound_update: Error in snd_pcm_recover\n");
			}

			/* If recovery fails, write silence and try again */
			if (silent_period) {
				snd_pcm_writei(handle, silent_period, period_size); /* Write silence */
			}
			snd_pcm_writei(handle, buffer, length); /* Write actual buffer */
		}
	}
}

/**
 * Terminates the sound system and releases resources.
 */
void sound_kill(void) {
	if (handle) {
		snd_pcm_drop(handle); /* Stop playback */
		snd_pcm_close(handle); /* Close the PCM device */
		snd_config_update_free_global(); /* Free global ALSA config */
		handle = NULL;
	}

	mem_free(silent_period); /* Free silent period buffer */
	silent_period = NULL;
}

/**
 * Pauses the sound system by stopping and closing the PCM device.
 */
void sound_pause(void) {
	sound_kill();
}

/**
 * Resumes playback by re-initializing the sound system.
 */
void sound_play(void) {
	sound_init();
}

/**
 * Sets the frame rate (currently a placeholder function).
 * 
 * @param fps The frame rate to set.
 */
void sound_setfps(int fps) {
	/* Placeholder for future frame rate adjustments */
}
