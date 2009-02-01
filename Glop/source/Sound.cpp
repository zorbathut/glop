// Includes
#include "Sound.h"
#include "System.h"
#include "fmod/fmod.hpp"

/*
// Globals
static SoundManager *gSoundManager = 0;
SoundManager *sound_manager() {return system()->sound_manager();}

// SoundSource
// ===========

void SoundSource::Play() {
  if (channel_ != 0)
    channel_->setPaused(false);
}

void SoundSource::Pause() {
  if (channel_ != 0)
    channel_->setPaused(true);
}

bool SoundSource::IsPaused() const {
  bool is_paused = false;
  if (channel_ != 0)
    channel_->getPaused(&is_paused);
  return is_paused;
}

void SoundSource::Stop() {
  if (channel_ != 0)
    channel_->stop();
}

bool SoundSource::IsStopped() const {
  bool is_playing = false;
  if (channel_ != 0)
    channel_->isPlaying(&is_playing);
  return !is_playing;
}

// SoundSample
// ===========

SoundSample *SoundSample::Load(BinaryFileReader reader, bool store_compressed, float base_volume) {
  // Load the data into memory - we could eventually switch to using extra_info.fileoffset or
  // FMOD_OPENUSER mode instead but this is good for now.
  if (!reader.IsOpen() || !sound_manager()->IsInitialized())
    return 0;
  int length = reader.GetLength();
  char *data = new char[length];
  reader.ReadChars(length, data);

  // Attempt to create the sound
  FMOD::Sound *sound;
  FMOD_RESULT result;
  FMOD_CREATESOUNDEXINFO extra_info;
  memset(&extra_info, 0, sizeof(extra_info));
  extra_info.cbsize = sizeof(extra_info);
  extra_info.length = length;
  if (store_compressed) {
    result = sound_manager()->GetSystem()->createSound(
      data, FMOD_CREATECOMPRESSEDSAMPLE | FMOD_OPENMEMORY, &extra_info, &sound);
  } else {
    result = sound_manager()->GetSystem()->createSound(
      data, FMOD_DEFAULT | FMOD_OPENMEMORY, &extra_info, &sound);
  }
  
  // Store the result or return error
  delete[] data;
  if (result != FMOD_OK)
    return 0;
  return new SoundSample(sound, base_volume);
}

SoundSample::~SoundSample() {
  sound_->release();
}

SoundSource SoundSample::Play(bool looped, bool start_paused) const {
  FMOD::Channel *channel;
  sound_->setLoopCount(looped? -1 : 0);
  sound_manager()->GetSystem()->playSound(FMOD_CHANNEL_FREE, sound_, start_paused, &channel);
  channel->setVolume(base_volume_);
  return SoundSource(channel);
}

// SoundManager
// ============

void SoundManager::SetVolume(float volume) {
  volume_ = volume;
  if (IsInitialized()) {
    FMOD::ChannelGroup *master_channel_group;
    system_->getMasterChannelGroup(&master_channel_group);
    master_channel_group->setVolume(volume);
  }
}

SoundManager::SoundManager()
: system_(0), volume_(1) {
  // Create the system object and check versions
  if (FMOD::System_Create(&system_) != FMOD_OK)
    return;
  unsigned int version;
  if (system_->getVersion(&version) != FMOD_OK || version < FMOD_VERSION)
    goto error;

  // Fix bad initialization options. FMOD documentation strongly recommends doing this for Windows.
  FMOD_CAPS caps;
  FMOD_SPEAKERMODE speaker_mode;
  if (system_->getDriverCaps(0, &caps, 0, 0, &speaker_mode) != FMOD_OK)
    goto error;
  if (system_->setSpeakerMode(speaker_mode) != FMOD_OK)
    goto error;
  if (caps & FMOD_CAPS_HARDWARE_EMULATED) {
    if (system_->setDSPBufferSize(1024, 10) != FMOD_OK)
      goto error;
  }

  // Initialize
  {
    const int kNumChannels = 128;
    if (system_->init(kNumChannels, FMOD_INIT_NORMAL, 0) != FMOD_OK) {
    if (system_->setSpeakerMode(FMOD_SPEAKERMODE_STEREO) != FMOD_OK)
      goto error;
    if (system_->init(kNumChannels, FMOD_INIT_NORMAL, 0) != FMOD_OK)
      goto error;
    }
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
    system_->release();
}

void SoundManager::Think() {
  if (IsInitialized())
    system_->update();
}

void SoundManager::PlayAllSources() {
  if (IsInitialized()) {
    FMOD::ChannelGroup *master_channel_group;
    system_->getMasterChannelGroup(&master_channel_group);
    master_channel_group->setPaused(false);
  }
}

void SoundManager::PauseAllSources() {
  if (IsInitialized()) {
    FMOD::ChannelGroup *master_channel_group;
    system_->getMasterChannelGroup(&master_channel_group);
    master_channel_group->setPaused(true);
  }
}

void SoundManager::StopAllSources() {
  if (IsInitialized()) {
    int num_channels;
    FMOD::ChannelGroup *master_channel_group;
    system_->getMasterChannelGroup(&master_channel_group);
    master_channel_group->getNumChannels(&num_channels);   
    for (int i = 0; i < num_channels; i++) {
      FMOD::Channel *channel;
      master_channel_group->getChannel(i, &channel);
      channel->stop();
    }
  }
}
*/

void SoundManager::Think() { }
SoundManager::SoundManager() { }
SoundManager::~SoundManager() { }
