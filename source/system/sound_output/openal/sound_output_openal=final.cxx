#include "types.h"
#include "misc/log.h"
#include "misc/config.h"
#include "misc/memutil.h"
#include "system/sound.h"

#include <AL/al.h>
#include <AL/alc.h>

#define kSoundBuffers 4

static ALCdevice* device = NULL;
static ALCcontext* context = NULL;
static ALuint buffers[kSoundBuffers];  /* Array of buffers */
static ALuint source;

static int buffersLoaded = 0;
static int initialized_audio = 0;

int sound_init()
{
    int i;

    if(config_get_bool("sound.enabled") == 0)
        return(0);

    log_printf("sound_init: Initializing audio.\n");

    /* Initialize the buffers to zero */
    for (i = 0; i < kSoundBuffers; i++) {
        buffers[i] = 0;
    }
    log_printf("sound_init: Buffers array initialized.\n");

    /* Open the audio device */
    device = alcOpenDevice(NULL);
    if (!device) {
        log_printf("sound_init: Failed to open audio device, Error: %s\n", alcGetString(NULL, alcGetError(device)));
        return 1;
    }
    log_printf("sound_init: OpenAL audio device opened successfully.\n");

    /* Log the device name */
    const char* device_name = alcGetString(device, ALC_DEVICE_SPECIFIER);
    log_printf("sound_init: OpenAL device name: %s\n", device_name ? device_name : "Unknown");

    /* Create the audio context */
    context = alcCreateContext(device, NULL);
    if (!context || !alcMakeContextCurrent(context)) {
        log_printf("sound_init: Failed to create OpenAL context, Error: %s\n", alcGetString(device, alcGetError(device)));
        alcCloseDevice(device);
        return 1;
    }
    log_printf("sound_init: OpenAL context created and set as current.\n");

    /* Log context attributes (only frequency available in OpenAL 1.1) */
    ALCint frequency;
    alcGetIntegerv(device, ALC_FREQUENCY, 1, &frequency);
    log_printf("sound_init: Context attributes: Frequency: %d Hz\n", frequency);

    /* Generate source */
    alGenSources(1, &source);
    if (alGetError() != AL_NO_ERROR) {
        log_printf("sound_init: Failed to generate OpenAL source, Error: %s\n", alGetString(alGetError()));
        alcDestroyContext(context);
        alcCloseDevice(device);
        return 1;
    }
    log_printf("sound_init: OpenAL source generated successfully.\n");

    /* Generate buffers */
    alGenBuffers(kSoundBuffers, buffers);  /* Generate multiple buffers */
    if (alGetError() != AL_NO_ERROR) {
        log_printf("sound_init: Failed to generate OpenAL buffers, Error: %s\n", alGetString(alGetError()));
        alcDestroyContext(context);
        alcCloseDevice(device);
        return 1;
    }
    log_printf("sound_init: OpenAL buffers generated successfully.\n");

    /* Log OpenAL version */
    const char* openal_version = (const char*)alGetString(AL_VERSION);
    log_printf("sound_init: OpenAL version: %s\n", openal_version);

    buffersLoaded = 0;
    initialized_audio = 1;

    log_printf("sound_init: Sound initialization complete.\n");
    return 0;
}

void sound_update(void *buffer_data, int frame_count)
{
    ALint state = 0;
    ALint processed = 0;
    int i;
    ALuint tmpbuffer;

    if (!initialized_audio) {
        return;
    }

    if (!buffersLoaded) {
        /* Initial buffer filling */
        for (i = 0; i < kSoundBuffers; i++) {
            alBufferData(buffers[i], AL_FORMAT_MONO16, buffer_data, frame_count * sizeof(s16), SAMPLE_RATE);
            if (alGetError() != AL_NO_ERROR) {
                log_printf("sound_update: Failed to fill buffer %d with sound data, Error: %s\n", i, alGetString(alGetError()));
            } else {
                log_printf("sound_update: Buffer %d filled with sound data.\n", i);
            }
        }

        alSourceQueueBuffers(source, kSoundBuffers, buffers);  /* Queue all buffers */
        if (alGetError() != AL_NO_ERROR) {
            log_printf("sound_update: Failed to queue buffers to source, Error: %s\n", alGetString(alGetError()));
        }
        buffersLoaded = 1;
    } else {
        processed = 0;
        alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);
        if (alGetError() != AL_NO_ERROR) {
            log_printf("sound_update: Failed to retrieve number of processed buffers, Error: %s\n", alGetString(alGetError()));
        }

        if (processed == 0) {
            return;
        }

        /* Unqueue and refill buffers */
        for (i = 0; i < processed; i++) {
            alSourceUnqueueBuffers(source, 1, &tmpbuffer);
            if (alGetError() != AL_NO_ERROR) {
                log_printf("sound_update: Failed to unqueue buffer, Error: %s\n", alGetString(alGetError()));
            }

            /* Refill the buffer with new sound data */
            alBufferData(tmpbuffer, AL_FORMAT_MONO16, buffer_data, frame_count * sizeof(s16), SAMPLE_RATE);
            if (alGetError() != AL_NO_ERROR) {
                log_printf("sound_update: Failed to refill buffer with sound data, Error: %s\n", alGetString(alGetError()));
            }

            /* Requeue the buffer */
            alSourceQueueBuffers(source, 1, &tmpbuffer);
            if (alGetError() != AL_NO_ERROR) {
                log_printf("sound_update: Failed to requeue buffer, Error: %s\n", alGetString(alGetError()));
            }
        }
    }

    /* Start playing the source if necessary */
    alGetSourcei(source, AL_SOURCE_STATE, &state);
    if (alGetError() != AL_NO_ERROR) {
        log_printf("sound_update: Failed to get source state, Error: %s\n", alGetString(alGetError()));
    }

    if (state != AL_PLAYING) {
        alSourcePlay(source);
        if (alGetError() != AL_NO_ERROR) {
            log_printf("sound_update: Failed to play source, Error: %s\n", alGetString(alGetError()));
        }
    }
}

void sound_kill()
{
    int i;

    if (source != 0) {
        alSourceStop(source);
        if (alGetError() != AL_NO_ERROR) {
            log_printf("sound_kill: Failed to stop sound source, Error: %s\n", alGetString(alGetError()));
        }
        alDeleteSources(1, &source);
        source = 0;
    }

    if (buffers[0] != 0) {  /* Check if buffers are valid */
        alDeleteBuffers(kSoundBuffers, buffers);
        if (alGetError() != AL_NO_ERROR) {
            log_printf("sound_kill: Failed to delete sound buffers, Error: %s\n", alGetString(alGetError()));
        }
        for (i = 0; i < kSoundBuffers; i++) {
            buffers[i] = 0;
        }
    }

    if (context != NULL) {
        alcMakeContextCurrent(NULL);
        alcDestroyContext(context);
        if (alcGetError(device) != ALC_NO_ERROR) {
            log_printf("sound_kill: Failed to destroy OpenAL context, Error: %s\n", alcGetString(device, alcGetError(device)));
        }
        context = NULL;
    }

    if (device != NULL) {
        alcCloseDevice(device);
        if (alcGetError(device) != ALC_NO_ERROR) {
            log_printf("sound_kill: Failed to close OpenAL device, Error: %s\n", alcGetString(NULL, alcGetError(device)));
        }
        device = NULL;
    }

    initialized_audio = 0;

    log_printf("sound_kill: OpenAL resources cleaned up.\n");
}

void sound_pause()
{
    if (!initialized_audio) {
        return;
    }

    if (source != 0) {
        alSourcePause(source);
        if (alGetError() != AL_NO_ERROR) {
            log_printf("sound_pause: Failed to pause sound source, Error: %s\n", alGetString(alGetError()));
        }
    }
}

void sound_play()
{
    if (!initialized_audio) {
        return;
    }

    if (source != 0) {
        alSourcePlay(source);
        if (alGetError() != AL_NO_ERROR) {
            log_printf("sound_play: Failed to play sound source, Error: %s\n", alGetString(alGetError()));
        }
    }
}

void sound_setfps(int fps)
{
    /* OpenAL handles audio playback at a fixed sample rate, so no FPS adjustment is needed.
       This function can be used for other purposes if required. */
}
