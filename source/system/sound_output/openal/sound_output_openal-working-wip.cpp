#include <AL/al.h>
#include <AL/alc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "types.h"
#include "misc/log.h"
#include "misc/config.h"
#include "misc/memutil.h"
#include "system/sound.h"

#define kSoundBuffers 4

static ALCdevice* device;
static ALCcontext* context;
static ALuint buffers[kSoundBuffers];  /* Array of buffers */
static ALuint source;

static int initialized;
static int buffersLoaded;
static int freq;
static int soundBufferLen;

static int initialized_audio = 0;

int sound_init()
{
    if (initialized_audio) {
        log_printf("sound_init: Audio already initialized.\n");
        return 0;  /* Already initialized */
    }

    log_printf("sound_init: Initializing audio.\n");

    /* Open the audio device */
    device = alcOpenDevice(NULL);
    if (!device) {
        log_printf("sound_init: Failed to open audio device, Error: %s\n", alcGetString(NULL, alcGetError()));
        return 1;
    }

    /* Create the audio context */
    context = alcCreateContext(device, NULL);
    if (!context) {
        log_printf("sound_init: Failed to create OpenAL context, Error: %s\n", alcGetString(device, alcGetError()));
        alcCloseDevice(device);
        return 1;
    }

    /* Make the context current */
    {
        ALCboolean retVal;
        retVal = alcMakeContextCurrent(context);
        if (retVal != ALC_TRUE) {
            log_printf("sound_init: Failed to make OpenAL context current, Error: %s\n", alcGetString(device, alcGetError()));
            alcDestroyContext(context);
            alcCloseDevice(device);
            return 1;
        }
    }

    /* Generate source and buffers */
    alGenSources(1, &source);
    if (alGetError() != AL_NO_ERROR) {
        log_printf("sound_init: Failed to generate OpenAL source, Error: %s\n", alGetString(alGetError()));
        alcDestroyContext(context);
        alcCloseDevice(device);
        return 1;
    }

    alGenBuffers(kSoundBuffers, buffers);  /* Generate multiple buffers */
    if (alGetError() != AL_NO_ERROR) {
        log_printf("sound_init: Failed to generate OpenAL buffers, Error: %s\n", alGetString(alGetError()));
        alcDestroyContext(context);
        alcCloseDevice(device);
        return 1;
    }

    freq = SAMPLE_RATE;
    soundBufferLen = (freq / 60) * 2;
    initialized = 1;
    buffersLoaded = 0;
    
    initialized_audio = 1;

    log_printf("sound_init: Sound initialization complete.\n");
    return 0;
}

void sound_update(void *buffer_data, int frame_count)
{
    (void)frame_count;  /* unused param */
    if (!initialized)
        return;

    {
        ALint sourceState = 0;
        ALint nBuffersProcessed = 0;

        if (!buffersLoaded) {
            /* Initial buffer filling */
            int i;
            for (i = 0; i < kSoundBuffers; i++) {
                alBufferData(buffers[i], AL_FORMAT_MONO16, buffer_data, soundBufferLen, freq);
                if (alGetError() != AL_NO_ERROR) {
                    log_printf("sound_update: Failed to fill buffer %d with sound data, Error: %s\n", i, alGetString(alGetError()));
                }
            }

            alSourceQueueBuffers(source, kSoundBuffers, buffers);  /* Queue all buffers */
            if (alGetError() != AL_NO_ERROR) {
                log_printf("sound_update: Failed to queue buffers to source, Error: %s\n", alGetString(alGetError()));
            }
            buffersLoaded = 1;
        } else {
            nBuffersProcessed = 0;
            alGetSourcei(source, AL_BUFFERS_PROCESSED, &nBuffersProcessed);
            if (alGetError() != AL_NO_ERROR) {
                log_printf("sound_update: Failed to retrieve number of processed buffers, Error: %s\n", alGetString(alGetError()));
            }

            if (nBuffersProcessed == 0) {
                return;
            }

            /* Unqueue and refill buffers */
            int i;
            for (i = 0; i < nBuffersProcessed; i++) {
                ALuint buffer;
                alSourceUnqueueBuffers(source, 1, &buffer);
                if (alGetError() != AL_NO_ERROR) {
                    log_printf("sound_update: Failed to unqueue buffer, Error: %s\n", alGetString(alGetError()));
                }

                /* Refill the buffer with new sound data */
                alBufferData(buffer, AL_FORMAT_MONO16, buffer_data, soundBufferLen, freq);
                if (alGetError() != AL_NO_ERROR) {
                    log_printf("sound_update: Failed to refill buffer with sound data, Error: %s\n", alGetString(alGetError()));
                }

                /* Requeue the buffer */
                alSourceQueueBuffers(source, 1, &buffer);
                if (alGetError() != AL_NO_ERROR) {
                    log_printf("sound_update: Failed to requeue buffer, Error: %s\n", alGetString(alGetError()));
                }
            }
        }

        /* Start playing the source if necessary */
        alGetSourcei(source, AL_SOURCE_STATE, &sourceState);
        if (alGetError() != AL_NO_ERROR) {
            log_printf("sound_update: Failed to get source state, Error: %s\n", alGetString(alGetError()));
        }

        if (sourceState != AL_PLAYING) {
            alSourcePlay(source);
            if (alGetError() != AL_NO_ERROR) {
                log_printf("sound_update: Failed to play source, Error: %s\n", alGetString(alGetError()));
            }
        }
    }
}

void sound_kill()
{
    if (source != 0)
    {
        alSourceStop(source);
        if (alGetError() != AL_NO_ERROR) {
            log_printf("sound_kill: Failed to stop sound source, Error: %s\n", alGetString(alGetError()));
        }
        alDeleteSources(1, &source);
        source = 0;
    }

    if (buffers[0] != 0)  /* Check if buffers are valid */
    {
        alDeleteBuffers(kSoundBuffers, buffers);
        if (alGetError() != AL_NO_ERROR) {
            log_printf("sound_kill: Failed to delete sound buffers, Error: %s\n", alGetString(alGetError()));
        }
        {
            int i;
            for (i = 0; i < kSoundBuffers; i++) {
                buffers[i] = 0;
            }
        }
    }

    if (context != NULL)
    {
        alcMakeContextCurrent(NULL);
        alcDestroyContext(context);
        if (alcGetError() != ALC_NO_ERROR) {
            log_printf("sound_kill: Failed to destroy OpenAL context, Error: %s\n", alcGetString(device, alcGetError()));
        }
        context = NULL;
    }

    if (device != NULL)
    {
        alcCloseDevice(device);
        if (alcGetError() != ALC_NO_ERROR) {
            log_printf("sound_kill: Failed to close OpenAL device, Error: %s\n", alcGetString(NULL, alcGetError()));
        }
        device = NULL;
    }

    initialized_audio = 0;

    log_printf("sound_kill: OpenAL resources cleaned up.\n");
}

void sound_pause()
{
    if (source != 0)
    {
        alSourcePause(source);
        if (alGetError() != AL_NO_ERROR)
        {
            log_printf("sound_pause: Failed to pause sound source, Error: %s\n", alGetString(alGetError()));
        }
    }
}

void sound_play()
{
    if (source != 0)
    {
        alSourcePlay(source);
        if (alGetError() != AL_NO_ERROR)
        {
            log_printf("sound_play: Failed to play sound source, Error: %s\n", alGetString(alGetError()));
        }
    }
}

void sound_setfps(int fps)
{
    /* OpenAL handles audio playback at a fixed sample rate, so no FPS adjustment is needed.
       This function can be used for other purposes if required. */
}
