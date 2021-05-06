#pragma once
#include <AL/al.h>
#include <AL/alc.h>
#include <sndfile.h>

#include <iostream>
#include <string>
#include <vector>

/**
 * I have never really touched OpenAL, so most of this is copypast from:
 * "Code, Tech, and Tutorials": https://www.youtube.com/channel/UC4EJN2OSNdl-mSxGjitRvyA
 * Repo: https://github.com/codetechandtutorials/openal-impl
 *
 */
class NotificationSound {
private:
    // Sound Device
    ALCdevice* m_device;
    ALCcontext* m_context;
    const ALCchar* m_device_name = nullptr;

    // Sound Playing
    ALuint m_source;
    float m_pitch       = 1.f;
    float m_gain        = 1.f;
    float m_position[3] = {0, 0, 0};
    float m_velocity[3] = {0, 0, 0};
    bool m_loop_sound   = false;
    ALuint m_buffer     = 0;

public:
    NotificationSound(const std::string& filename);
    ~NotificationSound();
    void play();
    void set_device();
    void set_sound_file(const std::string& filename);
};
