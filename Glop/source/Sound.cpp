// Includes
#include <cstring>
#include "Sound.h"
#include "System.h"
#include "fmod/fmod.h"

// Globals
static SoundManager *gSoundManager = 0;
SoundManager *sound_manager() {return system()->sound_manager();}

// SoundSource
// ===========

void SoundSource::Play() {
  if (channel_ != 0)
    FMOD_Channel_SetPaused(channel_, false);
}

void SoundSource::Pause() {
  if (channel_ != 0)
    FMOD_Channel_SetPaused(channel_, true);
}

bool SoundSource::IsPaused() const {
  FMOD_BOOL is_paused = false;
  if (channel_ != 0)
    FMOD_Channel_GetPaused(channel_, &is_paused);
  return is_paused;
}

void SoundSource::Stop() {
  if (channel_ != 0)
    FMOD_Channel_Stop(channel_);
}

bool SoundSource::IsStopped() const {
  FMOD_BOOL is_playing = false;
  if (channel_ != 0)
    FMOD_Channel_IsPlaying(channel_, &is_playing);
  return !is_playing;
}

// SoundSample
// ===========

SoundSample *SoundSample::Load(InputStream in, bool store_compressed, float base_volume) {
  // Load the data into memory - we could eventually switch to using extra_info.fileoffset or
  // FMOD_OPENUSER mode instead but this is good for now.
  if (!in.IsValid() || !sound_manager()->IsInitialized())
    return 0;
  char *data;
  int length = in.ReadAllData((void**)&data);

  // Attempt to create the sound
  FMOD_SOUND *sound;
  FMOD_RESULT result;
  FMOD_CREATESOUNDEXINFO extra_info;
  memset(&extra_info, 0, sizeof(extra_info));
  extra_info.cbsize = sizeof(extra_info);
  extra_info.length = length;
  if (store_compressed) {
    result = FMOD_System_CreateSound(sound_manager()->GetSystem(),
      data, FMOD_CREATECOMPRESSEDSAMPLE | FMOD_OPENMEMORY, &extra_info, &sound);
  } else {
    result = FMOD_System_CreateSound(sound_manager()->GetSystem(),
      data, FMOD_DEFAULT | FMOD_OPENMEMORY, &extra_info, &sound);
  }
  
  // Store the result or return error
  delete[] data;
  if (result != FMOD_OK)
    return 0;
  return new SoundSample(sound, base_volume);
}

SoundSample::~SoundSample() {
  FMOD_Sound_Release(sound_);
}

SoundSource SoundSample::Play(bool looped, bool start_paused) const {
  FMOD_CHANNEL *channel;
  FMOD_Sound_SetLoopCount(sound_, looped? -1 : 0);
  FMOD_System_PlaySound(sound_manager()->GetSystem(), FMOD_CHANNEL_FREE, sound_, start_paused, &channel);
  FMOD_Channel_SetVolume(channel, base_volume_);
  return SoundSource(channel);
}

// SoundManager
// ============

void SoundManager::SetVolume(float volume) {
  volume_ = volume;
  if (IsInitialized()) {
    FMOD_CHANNELGROUP *master_channel_group;
    FMOD_System_GetMasterChannelGroup(system_, &master_channel_group);
    FMOD_ChannelGroup_SetVolume(master_channel_group, volume);
  }
}

SoundManager::SoundManager()
: system_(0), volume_(1) {
  // Create the system object and check versions
  const int kNumChannels = 128;
  if (FMOD_System_Create(&system_) != FMOD_OK)
    return;
  unsigned int version;
  if (FMOD_System_GetVersion(system_, &version) != FMOD_OK || version < FMOD_VERSION)
    goto error;

  // Fix bad initialization options. FMOD documentation strongly recommends doing this for Windows.
  FMOD_CAPS caps;
  FMOD_SPEAKERMODE speaker_mode;
  if (FMOD_System_GetDriverCaps(system_, 0, &caps, 0, 0, &speaker_mode) != FMOD_OK)
    goto error;
  if (FMOD_System_SetSpeakerMode(system_, speaker_mode) != FMOD_OK)
    goto error;
  if (caps & FMOD_CAPS_HARDWARE_EMULATED) {
    if (FMOD_System_SetDSPBufferSize(system_, 1024, 10) != FMOD_OK)
      goto error;
  }

  // Initialize
  if (FMOD_System_Init(system_, kNumChannels, FMOD_INIT_NORMAL, 0) != FMOD_OK) {
    if (FMOD_System_SetSpeakerMode(system_, FMOD_SPEAKERMODE_STEREO) != FMOD_OK)
      goto error;
    if (FMOD_System_Init(system_, kNumChannels, FMOD_INIT_NORMAL, 0) != FMOD_OK)
      goto error;
  }

  // Success
  SetVolume(volume_);
  return;

  // Failure
error:;
  system_ = 0;
}

SoundManager::~SoundManager() {
  if (IsInitialized())
    FMOD_System_Release(system_);
}

void SoundManager::Think() {
  if (IsInitialized())
    FMOD_System_Update(system_);
}

void SoundManager::PlayAllSources() {
  if (IsInitialized()) {
    FMOD_CHANNELGROUP *master_channel_group;
    FMOD_System_GetMasterChannelGroup(system_, &master_channel_group);
    FMOD_ChannelGroup_SetPaused(master_channel_group, false);
  }
}

void SoundManager::PauseAllSources() {
  if (IsInitialized()) {
    FMOD_CHANNELGROUP *master_channel_group;
    FMOD_System_GetMasterChannelGroup(system_, &master_channel_group);
    FMOD_ChannelGroup_SetPaused(master_channel_group, true);
  }
}

void SoundManager::StopAllSources() {
  if (IsInitialized()) {
    int num_channels;
    FMOD_CHANNELGROUP *master_channel_group;
    FMOD_System_GetMasterChannelGroup(system_, &master_channel_group);
    FMOD_ChannelGroup_GetNumChannels(master_channel_group, &num_channels);   
    for (int i = 0; i < num_channels; i++) {
      FMOD_CHANNEL *channel;
      FMOD_ChannelGroup_GetChannel(master_channel_group, i, &channel);
      FMOD_Channel_Stop(channel);
    }
  }
}
