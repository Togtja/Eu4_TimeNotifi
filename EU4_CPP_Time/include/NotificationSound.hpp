#pragma once
#include <AL/al.h>
#include <AL/alc.h>
#include <sndfile.h>

#include <spdlog/spdlog.h>

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
    // void set_device() { devices = alcGetString(NULL, ALC_DEVICE_SPECIFIER); }
    std::vector<std::string> get_all_devices() {
        std::vector<std::string> all_devices;
        auto devices = alcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER);
        size_t from  = 0;
        for (size_t i = 0;; i++) {
            // Single null means end of that device
            if (devices[i] == '\0') {
                std::string device_string(devices + from);
                if (auto temp = alcOpenDevice(device_string.c_str()); temp) {
                    alcCloseDevice(temp);
                    all_devices.emplace_back(device_string);
                }
                // Double null means end of all devices
                if (devices[i + 1] == '\0') {
                    break;
                }
                from = i + 1;
            }
        }
        return all_devices;
    }

    bool set_device(const std::string& device) {
        auto temp_dev = alcOpenDevice(device.c_str());
        if (!temp_dev) {
            return false;
        }
        alcCloseDevice(m_device);

        alcMakeContextCurrent(nullptr);
        alcDestroyContext(m_context);
        m_device = temp_dev;

        m_context = alcCreateContext(m_device, nullptr);
        if (!m_context) {
            throw std::runtime_error("failed to create context");
        }

        if (!alcMakeContextCurrent(m_context)) {
            throw std::runtime_error("failed to make current context");
        }
        return true;
    }

    void set_sound_file(const std::string& filename);
};
