#include "types.h"
#include "misc/config.h"
#include "misc/memutil.h"
#include "misc/log.h"
#include "system/sound.h"
#include <AL/al.h>
#include <AL/alc.h>

#define NUM_BUFFERS 10

static ALCdevice *device = NULL;
static ALCcontext *context = NULL;
static ALuint buffers[NUM_BUFFERS];
static ALuint source;
static int initialized_audio = 0;

/* Helper function to check for OpenAL errors and log them */
static void check_al_error(const char *context)
{
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        log_printf("%s: OpenAL error 0x%04X\n", context, error);
    }
}

int sound_init()
{
    int i;

    /* Check if sound is enabled in the configuration */
    if (config_get_bool("sound.enabled") == 0) {
        return 0;  /* No need to initialize sound if it's disabled */
    }

    /* Open the default audio device */
    device = alcOpenDevice(NULL);
    if (!device) {
        log_printf("sound_init: Failed to open audio device.\n");
        return 1;
    }

    /* Create an OpenAL context */
    context = alcCreateContext(device, NULL);
    if (!context || !alcMakeContextCurrent(context)) {
        alcCloseDevice(device);
        log_printf("sound_init: Failed to create or make context current.\n");
        return 1;
    }

    /* Generate a source for audio playback */
    alGenSources(1, &source);
    check_al_error("sound_init: alGenSources");

    /* Generate buffers for storing sound data */
    alGenBuffers(NUM_BUFFERS, buffers);
    check_al_error("sound_init: alGenBuffers");

    /* Set source properties: front-center position and no distance attenuation */
    alSource3i(source, AL_POSITION, 0, 0, -1);
    alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
    alSourcei(source, AL_ROLLOFF_FACTOR, 0);

    /* Rewind the source and clear its buffer */
    alSourceRewind(source);
    alSourcei(source, AL_BUFFER, 0);

    /* Initialize buffers with silent data */
    for (i = 0; i < NUM_BUFFERS; i++) {
        alBufferData(buffers[i], AL_FORMAT_MONO16, NULL, BYTES_PER_FRAME, SAMPLE_RATE);
        check_al_error("sound_init: alBufferData");
    }

    /* Queue the buffers for playback */
    alSourceQueueBuffers(source, NUM_BUFFERS, buffers);
    check_al_error("sound_init: alSourceQueueBuffers");

    /* Start playback of the source */
    alSourcePlay(source);
    check_al_error("sound_init: alSourcePlay");

    initialized_audio = 1;  /* Mark audio as initialized */

    return 0;
}

void sound_update(void *buffer_data, int frame_count)
{
    ALint state = 0;
    ALint processed = 0;
    ALuint buffer = 0;

    /* If audio is not initialized, do nothing */
    if (!initialized_audio) {
        return;
    }

    /* Check current state of the audio source */
    alGetSourcei(source, AL_SOURCE_STATE, &state);
    alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);

    /* If no buffers have been processed, return early */
    if (processed == 0) {
        return;
    }

    /* Unqueue a processed buffer and refill it with new sound data */
    alSourceUnqueueBuffers(source, 1, &buffer);
    check_al_error("sound_update: alSourceUnqueueBuffers");

    /* Fill the buffer with the new audio data */
    alBufferData(buffer, AL_FORMAT_MONO16, buffer_data, frame_count * SAMPLE_SIZE, SAMPLE_RATE);
    check_al_error("sound_update: alBufferData");

    /* Queue the buffer back for playback */
    alSourceQueueBuffers(source, 1, &buffer);
    check_al_error("sound_update: alSourceQueueBuffers");

    /* If the source isn't playing, start playback */
    if (state != AL_PLAYING) {
        alSourcePlay(source);
        check_al_error("sound_update: alSourcePlay");
    }
}

void sound_kill()
{
    int i;

    /* Stop and delete the audio source */
    if (source != 0) {
        alSourceStop(source);
        alDeleteSources(1, &source);
        source = 0;
    }

    /* Delete the audio buffers */
    if (buffers[0] != 0) {
        alDeleteBuffers(NUM_BUFFERS, buffers);
        for (i = 0; i < NUM_BUFFERS; i++) {
            buffers[i] = 0;
        }
    }

    /* Clean up the OpenAL context and device */
    if (context != NULL) {
        alcMakeContextCurrent(NULL);
        alcDestroyContext(context);
        context = NULL;
    }

    if (device != NULL) {
        alcCloseDevice(device);
        device = NULL;
    }

    initialized_audio = 0;  /* Mark audio as uninitialized */
}

void sound_pause()
{
    /* Pause playback if audio is initialized */
    if (initialized_audio && source != 0) {
        alSourcePause(source);
        check_al_error("sound_pause: alSourcePause");
    }
}

void sound_play()
{
    /* Resume playback if audio is initialized */
    if (initialized_audio && source != 0) {
        alSourcePlay(source);
        check_al_error("sound_play: alSourcePlay");
    }
}

void sound_setfps(int fps)
{
    /* Placeholder function, not currently used */
}
