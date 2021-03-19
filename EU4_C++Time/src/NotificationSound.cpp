#include <NotificationSound.hpp>

#include <filesystem>
NotificationSound::NotificationSound(const std::string& filename) {
    
    // Init devices
    m_device = alcOpenDevice(nullptr);
    if(!m_device){
        throw std::runtime_error("failed to get sound device");
    }

    m_context = alcCreateContext(m_device, nullptr);
    if(!m_context){
        throw std::runtime_error("failed to create context");
    }

    if(!alcMakeContextCurrent(m_context)){
        throw std::runtime_error("failed to make current context");
    }


	if (alcIsExtensionPresent(m_device, "ALC_ENUMERATE_ALL_EXT"))
		m_device_name = alcGetString(m_device, ALC_ALL_DEVICES_SPECIFIER);
	if (!m_device_name || alcGetError(m_device) != AL_NO_ERROR)
		m_device_name = alcGetString(m_device, ALC_DEVICE_SPECIFIER);

    //Load sound file and Init sound buffer
    if(!std::filesystem::exists(filename)){
        throw std::runtime_error("failed: file " + filename + " does not exist");
    }

    SF_INFO sfinfo;
    sfinfo.format = SF_FORMAT_OGG;
    SNDFILE* sndfile = sf_open(std::filesystem::absolute(filename).string().c_str(), SFM_READ, &sfinfo);
    if (!sndfile) {
        auto err = sf_error(sndfile);
        auto errS = sf_error_number(err);
		throw std::runtime_error("failed to open " + std::filesystem::absolute(filename).string() + "due to: " + errS);
	}

	if (sfinfo.frames < 1 || sfinfo.frames >(sf_count_t)(INT_MAX / sizeof(short)) / sfinfo.channels) {
        
		throw std::runtime_error("bad sample count in " + filename + "(" + std::to_string(sfinfo.frames) + ")");
		sf_close(sndfile);
	}

    ALenum format = AL_NONE;
    if(sfinfo.channels == 1) {
        format = AL_FORMAT_MONO16;
    }
    else if(sfinfo.channels == 2) {
        format = AL_FORMAT_STEREO16;
    }
    if (!format){
		throw std::runtime_error("Unsupported channel count: " +  std::to_string(sfinfo.channels));
		sf_close(sndfile);
    }


    short* membuff = new short[sfinfo.frames * sfinfo.channels];
    auto num_frames = sf_readf_short(sndfile, membuff, sfinfo.frames);
    if(num_frames < 1){
        delete[] membuff;
        sf_close(sndfile);
        throw std::runtime_error("failed to read samples in " + filename + "(" + std::to_string(num_frames) + ")");
    } 
    ALsizei num_bytes = static_cast<ALsizei>(num_frames * sfinfo.channels * sizeof(short));

    m_buffer = 0;
    alGenBuffers(1, &m_buffer);
    alBufferData(m_buffer, format, membuff, num_bytes, sfinfo.samplerate);
    
    delete[] membuff;
    sf_close(sndfile);

    auto err = alGetError();
    auto err_str = alGetString(err);
    const char* err_c = static_cast<const char*>(err_str);
    if(!err_str){
        throw std::runtime_error("openAL failure: " + std::string(err_c));
    }

    if(err != AL_NO_ERROR){
        if(m_buffer && alIsBuffer(m_buffer)){
            alDeleteBuffers(1, &m_buffer);
        }
        throw std::runtime_error("openAL failure: " + std::string(alGetString(err)));
    }


    //Init sound
    alGenSources(1, &m_source);
    alSourcef(m_source, AL_PITCH, m_pitch);
	alSourcef(m_source, AL_GAIN, m_gain);
	alSource3f(m_source, AL_POSITION, m_position[0], m_position[1], m_position[2]);
	alSource3f(m_source, AL_VELOCITY, m_velocity[0], m_velocity[1], m_velocity[2]);
	alSourcei(m_source, AL_LOOPING, m_loop_sound);
	alSourcei(m_source, AL_BUFFER, m_buffer);
}

NotificationSound::~NotificationSound() {
    //TODO check succsess
    alcMakeContextCurrent(nullptr);
    alcDestroyContext(m_context);
    alDeleteSources(1, &m_source);

    alDeleteBuffers(1, &m_buffer);
    alcCloseDevice(m_device);

}

void NotificationSound::play() {
    alSourcePlay(m_source);
    ALint state = AL_PLAYING;
    while (state == AL_PLAYING && alGetError() == AL_NO_ERROR) {
		alGetSourcei(m_source, AL_SOURCE_STATE, &state);
	}

}
