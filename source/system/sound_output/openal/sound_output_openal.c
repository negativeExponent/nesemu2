#include "types.h"
#include "misc/config.h"
#include "misc/memutil.h"
#include "system/sound.h"
#include "misc/log.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <time.h>

#define NUM_BUFFERS 4

static ALCdevice *device = NULL;
static ALCcontext *context = NULL;
static ALuint buffers[NUM_BUFFERS];
static ALuint source;

static int buffersLoaded = 0;
static int initialized_audio = 0;

int sound_init()
{
	int i;

	if (config_get_bool("sound.enabled") == 0) {
		return 0;
	}

	device = alcOpenDevice(NULL);
	if (!device) {
		return 1;
	}

	context = alcCreateContext(device, NULL);
	if (!context || !alcMakeContextCurrent(context)) {
		alcCloseDevice(device);
		return 1;
	}

	alGenSources(1, &source);
	if (alGetError() != AL_NO_ERROR) {
		alcDestroyContext(context);
		alcCloseDevice(device);
		return 1;
	}

	alGenBuffers(NUM_BUFFERS, buffers);
	if (alGetError() != AL_NO_ERROR) {
		alcDestroyContext(context);
		alcCloseDevice(device);
		return 1;
	}

	for (i = 0; i < NUM_BUFFERS; i++) {
		alBufferData(buffers[i], AL_FORMAT_MONO16, NULL, 0, SAMPLE_RATE);
		if (alGetError() != AL_NO_ERROR) {
			alcDestroyContext(context);
			alcCloseDevice(device);
			return 1;
		}
	}

	alSourceQueueBuffers(source, NUM_BUFFERS, buffers);

	buffersLoaded = 0;
	initialized_audio = 1;

	return 0;
}

void sound_update(void *buffer_data, int frame_count)
{
	ALint state = 0;
	ALint processed = 0;

	if (!initialized_audio) {
		return;
	}

	alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);

	if (processed > 0) {
		int i;
		ALuint buffer = 0;
		for (i = 0; i < processed; i++) {
			alSourceUnqueueBuffers(source, 1, &buffer);
			alBufferData(buffer, AL_FORMAT_MONO16, buffer_data, frame_count * SAMPLE_SIZE, SAMPLE_RATE);
			alSourceQueueBuffers(source, 1, &buffer);
		}
	}

	alGetSourcei(source, AL_SOURCE_STATE, &state);
	if (state != AL_PLAYING) {
		alSourcePlay(source);
	}
}

void sound_kill()
{
	if (source != 0) {
		alSourceStop(source);
		alDeleteSources(1, &source);
		source = 0;
	}

	if (buffers[0] != 0) {
		int i;
		alDeleteBuffers(NUM_BUFFERS, buffers);
		for (i = 0; i < NUM_BUFFERS; i++) {
			buffers[i] = 0;
		}
	}

	if (context != NULL) {
		alcMakeContextCurrent(NULL);
		alcDestroyContext(context);
		context = NULL;
	}

	if (device != NULL) {
		alcCloseDevice(device);
		device = NULL;
	}

	initialized_audio = 0;
}

void sound_pause()
{
	if (!initialized_audio) {
		return;
	}

	if (source != 0) {
		alSourcePause(source);
	}
}

void sound_play()
{
	if (!initialized_audio) {
		return;
	}

	if (source != 0) {
		alSourcePlay(source);
	}
}

void sound_setfps(int fps)
{
}
