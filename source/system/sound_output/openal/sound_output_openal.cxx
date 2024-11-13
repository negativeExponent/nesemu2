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

static ALCdevice *device = NULL;
static ALCcontext *context = NULL;
static ALuint source = 0;
static ALuint buffer = 0;

static int initialized_audio = 0;

void log_error(const char *message) {
    log_printf("Error: %s\n", message);
}

int sound_init()
{
    if (initialized_audio) {
        return 0;  // Already initialized
    }

    // Open the default audio device
    device = alcOpenDevice(NULL); // NULL = default device
    if (!device) {
        log_error("alcOpenDevice() failed!");
        return 1;
    }

    // Create a context and make it current
    context = alcCreateContext(device, NULL);
    if (!context || !alcMakeContextCurrent(context)) {
        log_error("alcCreateContext() or alcMakeContextCurrent() failed!");
        alcCloseDevice(device);
        return 1;
    }

    // Generate the OpenAL source and buffer
    alGenSources(1, &source);
    if (alGetError() != AL_NO_ERROR) {
        log_error("alGenSources() failed!");
        sound_kill();
        return 1;
    }

    alGenBuffers(1, &buffer);
    if (alGetError() != AL_NO_ERROR) {
        log_error("alGenBuffers() failed!");
        sound_kill();
        return 1;
    }

    // Set initial playback properties for the source
    alSourcei(source, AL_LOOPING, AL_FALSE);
    if (alGetError() != AL_NO_ERROR) {
        log_error("alSourcei() failed to set properties!");
    }

    initialized_audio = 1;

    sound_play();

    printf("sound_init: Sound initialization done.\n");
    return 0;
}

void sound_update(void *buffer_data, int frame_count)
{
    if (!initialized_audio) return;

    if (!buffer_data || frame_count <= 0)
    {
        log_printf("OpenAL: Invalid buffer data or frame count.\n");
        return;
    }

    // Calculate the length in bytes (16-bit mono = 2 bytes per frame)
    int length = frame_count * 2;  // 2 bytes per frame for 16-bit mono

    // Ensure the length is in bytes (AL_BufferData requires length in bytes)
    ALenum format = AL_FORMAT_MONO16;  // 16-bit mono format

    // Log the buffer data size and ensure it's correct
    // log_printf("OpenAL: Updating buffer with %d bytes of data.\n", length);

    // Load the buffer with audio data (SAMPLE_RATE is set to 44100Hz)
    alBufferData(buffer, format, buffer_data, length, SAMPLE_RATE);
    if (alGetError() != AL_NO_ERROR)
    {
        log_printf("OpenAL: alBufferData() failed! Error: %s\n", alGetString(alGetError()));
        return;
    }

    // Attach the buffer to the source
    alSourcei(source, AL_BUFFER, buffer);
    if (alGetError() != AL_NO_ERROR)
    {
        log_printf("OpenAL: alSourcei() failed! Error: %s\n", alGetString(alGetError()));
        return;
    }

    // Ensure the source is playing continuously by checking playback state and re-triggering if needed
    ALint state;
    alGetSourcei(source, AL_SOURCE_STATE, &state);
    if (state != AL_PLAYING)
    {
        alSourcePlay(source);
        if (alGetError() != AL_NO_ERROR)
        {
            log_printf("OpenAL: alSourcePlay() failed! Error: %s\n", alGetString(alGetError()));
        }
    }
}

void sound_kill()
{
    if (source != 0)
    {
        alSourceStop(source);
        alDeleteSources(1, &source);
        source = 0;
    }

    if (buffer != 0)
    {
        alDeleteBuffers(1, &buffer);
        buffer = 0;
    }

    if (context != NULL)
    {
        alcMakeContextCurrent(NULL);
        alcDestroyContext(context);
        context = NULL;
    }

    if (device != NULL)
    {
        alcCloseDevice(device);
        device = NULL;
    }

    initialized_audio = 0;

    log_printf("OpenAL: Resources cleaned up.\n");
}

void sound_pause()
{
    if (source != 0)
    {
        alSourcePause(source);
        if (alGetError() != AL_NO_ERROR)
        {
            log_printf("OpenAL: alSourcePause() failed! Error: %s\n", alGetString(alGetError()));
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
            log_printf("OpenAL: alSourcePlay() failed! Error: %s\n", alGetString(alGetError()));
        }
    }
}

void sound_setfps(int fps)
{
    // OpenAL handles audio playback at a fixed sample rate, so no FPS adjustment is needed.
    // This function can be used for other purposes if required.
}
