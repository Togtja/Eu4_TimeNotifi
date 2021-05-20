#pragma once
#include <AL/al.h>
#include <AL/alc.h>
#include <sndfile.h>

#include <spdlog/spdlog.h>

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
    ALCdevice* m_device          = nullptr;
    ALCcontext* m_context        = nullptr;
    const ALCchar* m_device_name = nullptr;

    // Sound Playing
    ALuint m_source{};
    float m_pitch       = 1.f;
    float m_gain        = 1.f;
    float m_position[3] = {0, 0, 0};
    float m_velocity[3] = {0, 0, 0};
    bool m_loop_sound   = false;
    ALuint m_buffer     = 0;

    // sound file
    std::string m_filename{};

    void init();

    void deinit();

public:
    NotificationSound(const std::string& filename);
    NotificationSound(const std::string& filename, const std::string& device);
    ~NotificationSound();

    void play();

    std::vector<std::string> get_all_devices();

    bool set_device(const std::string& device);
    std::string get_device() { return std::string(m_device_name); }

    void set_sound_file(const std::string& filename);
};
