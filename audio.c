/*
    Labyrinth
    Benedict Henshaw, 2018
    audio.c - Real-time mixing and playback of audio.
*/

typedef struct
{
    f32 * samples;
    int sample_count;
}
Sound;

typedef struct
{
    f32 * samples;
    int sample_count;
    int sample_index;
    f32 left_gain;
    f32 right_gain;
    bool loop;
    bool playing;
}
Mixer_Channel;

typedef struct
{
    Mixer_Channel * channels;
    int channel_count;
    f32 gain;
}
Mixer;

Mixer create_mixer(int pool_index, int channel_count, f32 gain)
{
    Mixer mixer = {};
    mixer.channels = malloc(channel_count * sizeof(Mixer_Channel));
    if (mixer.channels)
    {
        mixer.channel_count = channel_count;
        mixer.gain = gain;
    }
    return mixer;
}

void mix_audio(Mixer * mixer, void * stream, int samples_requested)
{
    f32 * samples = stream;
    for (int sample_index = 0;
        sample_index < samples_requested;
        ++sample_index)
    {
        samples[sample_index] = 0.0f;
    }

    for (int channel_index = 0; channel_index < mixer->channel_count; ++channel_index)
    {
        Mixer_Channel * channel = &mixer->channels[channel_index];
        if (channel->samples && channel->playing)
        {
            for (int sample_index = 0;
                 sample_index < samples_requested &&
                 channel->sample_index < channel->sample_count;
                 ++sample_index)
            {
                f32 new_left  = channel->samples[channel->sample_index];
                f32 new_right = channel->samples[channel->sample_index];

                new_left  *= channel->left_gain;
                new_left  *= mixer->gain;
                new_right *= channel->right_gain;
                new_right *= mixer->gain;

                samples[sample_index] += new_left;
                ++sample_index;
                samples[sample_index] += new_right;

                channel->sample_index += 1;
            }

            if (channel->sample_index >= channel->sample_count)
            {
                if (channel->loop)
                {
                    channel->sample_index = 0;
                }
                else
                {
                    *channel = (Mixer_Channel){};
                }
            }
        }
    }
}

int play_sound(Mixer * mixer, Sound sound, f32 left_gain, f32 right_gain, int loop)
{
    for (int i = 0; i < mixer->channel_count; ++i)
    {
        if (mixer->channels[i].samples == NULL)
        {
            SDL_LockAudioDevice(audio_device);
            mixer->channels[i].samples      = sound.samples;
            mixer->channels[i].sample_count = sound.sample_count;
            mixer->channels[i].sample_index = 0;
            mixer->channels[i].left_gain    = left_gain;
            mixer->channels[i].right_gain   = right_gain;
            mixer->channels[i].loop         = loop;
            mixer->channels[i].playing      = true;
            SDL_UnlockAudioDevice(audio_device);
            return i;
        }
    }
    return -1;
}

int queue_sound(Mixer * mixer, Sound sound, f32 left_gain, f32 right_gain, bool loop)
{
    for (int i = 0; i < mixer->channel_count; ++i)
    {
        if (mixer->channels[i].samples == NULL)
        {
            SDL_LockAudioDevice(audio_device);
            mixer->channels[i].samples      = sound.samples;
            mixer->channels[i].sample_count = sound.sample_count;
            mixer->channels[i].sample_index = 0;
            mixer->channels[i].left_gain    = left_gain;
            mixer->channels[i].right_gain   = right_gain;
            mixer->channels[i].loop         = loop;
            mixer->channels[i].playing      = false;
            SDL_UnlockAudioDevice(audio_device);
            return i;
        }
    }
    return -1;
}

bool play_channel(Mixer * mixer, int channel_index)
{
    if (channel_index >= 0 && channel_index <= mixer->channel_count && mixer->channels[channel_index].samples)
    {
        SDL_LockAudioDevice(audio_device);
        mixer->channels[channel_index].playing = true;
        SDL_UnlockAudioDevice(audio_device);
        return true;
    }
    return false;
}

bool pause_channel(Mixer * mixer, int channel_index)
{
    if (channel_index >= 0 &&
        channel_index <= mixer->channel_count &&
        mixer->channels[channel_index].samples)
    {
        SDL_LockAudioDevice(audio_device);
        mixer->channels[channel_index].playing = false;
        SDL_UnlockAudioDevice(audio_device);
        return true;
    }
    return false;
}

bool stop_sound(Mixer * mixer, Sound sound)
{
    for (int i = 0; i < mixer->channel_count; ++i)
    {
        if (mixer->channels[i].samples == sound.samples)
        {
            mixer->channels[i] = (Mixer_Channel){};
            return true;
        }
    }
    return false;
}

bool sound_is_playing(Mixer * mixer, Sound sound)
{
    for (int i = 0; i < mixer->channel_count; ++i)
    {
        if (mixer->channels[i].samples == sound.samples)
        {
            return true;
        }
    }
    return false;
}
