#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/unistd.h>
#include <pulse/pulseaudio.h>

#include "types.h"
#include "misc/log.h"
#include "misc/config.h"
#include "misc/memutil.h"
#include "system/sound.h"

/* Enum for various flags */
typedef enum {
	FLAG_SUCCESS = 1 << 0,	/* Flag for success/failure of PulseAudio operations */
	FLAG_NONBLOCK = 1 << 1,	/* Flag for non-blocking mode in stream writes */
	FLAG_PAUSED = 1 << 2	/* Flag for stream paused status */
} Flag;

static int flags = 0;	/* Bitwise flags variable */

/* Helper macros to set/get flags */
#define SET_FLAG(flag)		(flags |= (flag))
#define CLEAR_FLAG(flag)	(flags &= ~(flag))
#define CHECK_FLAG(flag)	(flags & (flag))

static pa_threaded_mainloop *mainloop = NULL;
static pa_stream *stream = NULL;
static pa_context *context = NULL;

static int initialized_audio = 0;
static int snd_enabled = 1;

static void context_state_callback(pa_context *c, void *userdata)
{
	pa_context_state_t state = pa_context_get_state(c);
	switch (state) {
	case PA_CONTEXT_READY:
	case PA_CONTEXT_FAILED:
	case PA_CONTEXT_TERMINATED:
		pa_threaded_mainloop_signal(mainloop, 0);
		break;
	default:
		break;
	}
}

static void stream_state_cb(pa_stream *s, void *data)
{
	pa_stream_state_t stream_state = pa_stream_get_state(s);
	switch (stream_state) {
	case PA_STREAM_READY:
	case PA_STREAM_FAILED:
	case PA_STREAM_TERMINATED:
		break;
	default:
		break;
	}
}

static void stream_write_callback(pa_stream *s, size_t length, void *userdata)
{
	// This callback is triggered when PulseAudio is ready to write more data.
	// If needed, handle buffer management or backpressure here.
	pa_threaded_mainloop_signal(mainloop, 0);
}

static void stream_success_callback(pa_stream *s, int success, void *data)
{
	(void)s;

	if (success)
		SET_FLAG(FLAG_SUCCESS);
	else
		CLEAR_FLAG(FLAG_SUCCESS);
	
	pa_threaded_mainloop_signal(mainloop, 0);
}

int sound_init()
{
	pa_sample_spec ss;
	pa_buffer_attr paattr;
	const pa_buffer_attr *server_attr = NULL;

	int buffer_size;
	int latency_ms = 64;
	int ret;

	if (config_get_bool("sound.enabled") == 0)
		return 0;
	

	if (initialized_audio)
		sound_kill();

	log_printf("sound_init: Start initialization\n");

	mainloop = pa_threaded_mainloop_new();
	if (!mainloop) {
		log_printf("PulseAudio: Failed to create mainloop\n");
		return -1;
	}

	context = pa_context_new(pa_threaded_mainloop_get_api(mainloop), "nesemu2");
	if (!context) {
		log_printf("PulseAudio: Failed to create context\n");
		return -1;
	}

	pa_context_set_state_callback(context, context_state_callback, NULL);

	ret = pa_context_connect(context, NULL, PA_CONTEXT_NOFLAGS, NULL);
	if (ret < 0) {
		log_printf("PulseAudio: Failed to connect to server (Error %d)\n", ret);
		return -1;
	}

	pa_threaded_mainloop_lock(mainloop);

	ret = pa_threaded_mainloop_start(mainloop);
	if (ret < 0) {
		log_printf("PulseAudio: Failed to start mainloop (Error %d)\n", ret);
		return -1;
	}

	pa_threaded_mainloop_wait(mainloop);

	if (pa_context_get_state(context) != PA_CONTEXT_READY) {
		log_printf("PulseAudio: Timeout for context state!\n");
		pa_threaded_mainloop_unlock(mainloop);
		return -1;
	}

	ss.format = PA_SAMPLE_S16LE;
	ss.channels = CHANNELS;
	ss.rate = SAMPLE_RATE;

	stream = pa_stream_new(context, "SMS Plus GX", &ss, NULL);
	if (!stream) {
		log_printf("PulseAudio: Failed to create stream\n");
		return -1;
	}

	pa_stream_set_state_callback(stream, stream_state_cb, NULL);
	pa_stream_set_write_callback(stream, stream_write_callback, NULL);

	paattr.tlength = ss.rate * sizeof(s16) * latency_ms / 1000;
	paattr.prebuf = paattr.tlength;
	paattr.maxlength = paattr.tlength * 2;
	paattr.minreq = paattr.tlength / 2;
	paattr.fragsize = paattr.tlength / 4;

	ret = pa_stream_connect_playback(stream, NULL, &paattr, PA_STREAM_NOFLAGS, NULL, NULL);
	if (ret < 0) {
		log_printf("PulseAudio: Failed to connect stream for playback (Error %d)\n", ret);
		return -1;
	}

	pa_threaded_mainloop_wait(mainloop);

	server_attr = pa_stream_get_buffer_attr(stream);
	if (server_attr) {
		buffer_size = server_attr->tlength;
		log_printf("sound_init: Requested %u bytes buffer, got %u.\n",
			(unsigned)paattr.tlength,
			(unsigned)buffer_size);
	} else {
		buffer_size = paattr.tlength;
	}

	pa_threaded_mainloop_unlock(mainloop);

	log_printf("sound_init: Sample rate: %u Hz\n", ss.rate);
	log_printf("sound_init: Channels: %d\n", ss.channels);
	log_printf("sound_init: Buffer size: %u bytes\n", buffer_size);

	log_printf("sound_init: Initialization done\n");

	CLEAR_FLAG(FLAG_NONBLOCK); /* block mode by default */
	CLEAR_FLAG(FLAG_PAUSED);
	SET_FLAG(FLAG_SUCCESS);

	initialized_audio = 1;
	snd_enabled = 1;

	return 0;
}

void sound_update(void *buffer, int length)
{
	size_t buffer_size = length * sizeof(s16); /* length is in frames, we need size in bytes */
	const uint8_t *buf = (const uint8_t*)buffer;
	size_t written = 0;

	/* Lock the main loop before interacting with PulseAudio */
	pa_threaded_mainloop_lock(mainloop);
	while (buffer_size) {
		size_t writable = MIN(buffer_size, pa_stream_writable_size(stream));	/* Calculate how much data can be written */

		if (writable) {
			pa_stream_write(stream, buf, writable, NULL, 0, PA_SEEK_RELATIVE);
			buf += writable;
			buffer_size -= writable;
			written += writable;
		} else if (!CHECK_FLAG(FLAG_NONBLOCK)) {
			pa_threaded_mainloop_wait(mainloop);
		} else {
			break;
		}
	}

	pa_threaded_mainloop_unlock(mainloop);

	/*if (written < length) {
		log_printf("sound_update: Failed to write the entire buffer, %zu bytes written.\n", written);
	}*/
}

void sound_kill()
{
	if (stream) {
		pa_stream_disconnect(stream);
		pa_stream_unref(stream);
	}

	if (context) {
		pa_context_disconnect(context);
		pa_context_unref(context);
	}

	if (mainloop) {
		pa_threaded_mainloop_stop(mainloop);
		pa_threaded_mainloop_free(mainloop);
	}
}

void sound_pause()
{
	if (initialized_audio) {
		pa_threaded_mainloop_lock(mainloop);
		pa_stream_cork(stream, 1, stream_success_callback, NULL);
		pa_threaded_mainloop_wait(mainloop);
		pa_threaded_mainloop_unlock(mainloop);
	}
}

void sound_play()
{
	if (initialized_audio) {
		pa_threaded_mainloop_lock(mainloop);
		pa_stream_cork(stream, 0, stream_success_callback, NULL);
		pa_threaded_mainloop_wait(mainloop);
		pa_threaded_mainloop_unlock(mainloop);
	}
}

void sound_setfps(int fps)
{
}
