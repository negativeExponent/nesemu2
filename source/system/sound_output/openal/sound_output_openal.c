#include <unistd.h>
#include "types.h"
#include "misc/config.h"
#include "misc/memutil.h"
#include "misc/log.h"
#include "system/sound.h"
#include <AL/al.h>
#include <AL/alc.h>

#define NUM_BUFFERS 4

static ALCdevice *device = NULL;
static ALCcontext *context = NULL;
static ALuint buffers[NUM_BUFFERS];
static ALuint source;
static int initialized_audio = 0;

#ifdef _WIN32
#define timer_wait(msec) (Sleep((msec)))
#else
#include <time.h>
static void timer_wait(int msec)
{
    struct timespec tv;
    tv.tv_sec          = msec / 1000;
    tv.tv_nsec         = (msec % 1000) * 1000000;
    nanosleep(&tv, NULL);
}
#endif

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
    u8* silent_buffer = 0;
    int silent_buffer_size;

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
    silent_buffer_size = BYTES_PER_FRAME;
    silent_buffer = mem_realloc(silent_buffer, silent_buffer_size);
    for (i = 0; i < NUM_BUFFERS; i++) {
        alBufferData(buffers[i], AL_FORMAT_MONO16, silent_buffer, silent_buffer_size, SAMPLE_RATE);
    }
    alSourceQueueBuffers(source, NUM_BUFFERS, buffers);

    mem_free(silent_buffer);

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
    ALuint buffer_id = 0;
    u32 count = 0;

    /* If audio is not initialized, do nothing */
    if (!initialized_audio) {
        return;
    }

    /* Loop to handle processed buffers */
    while (count < 60) {
        /* Get the number of buffers that have been processed */
        alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);
        check_al_error("sound_update: alGetSourcei");

        /* If there are processed buffers, handle them */
        if (processed > 0) {
            log_printf("processed = %d\n", processed);
            /* Unqueue a processed buffer and fill it with new data */
            alSourceUnqueueBuffers(source, 1, &buffer_id);
            check_al_error("sound_update: alSourceUnqueueBuffers");

            /* Fill the buffer with new sound data */
            alBufferData(buffer_id, AL_FORMAT_MONO16, buffer_data, frame_count * SAMPLE_SIZE, SAMPLE_RATE);
            check_al_error("sound_update: alBufferData");

            /* Queue the buffer back for playback */
            alSourceQueueBuffers(source, 1, &buffer_id);
            check_al_error("sound_update: alSourceQueueBuffers");

            break;  /* Exit the loop once we've processed one buffer */
        }

        /* If no buffers have been processed, wait for a bit */
        timer_wait(1);  /* Sleep for a small period to avoid busy waiting */
        count++;
    }

    /* If the audio source is not currently playing, start it */
    alGetSourcei(source, AL_SOURCE_STATE, &state);
    check_al_error("sound_update: alGetSourcei");
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
