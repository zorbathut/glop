// Load and play sounds.

#ifndef GLOP_SOUND_H__
#define GLOP_SOUND_H__

// Includes
#include "Base.h"
#include "BinaryFileManager.h"

// Class declarations
class SoundManager;
namespace FMOD {
class Channel;
class Sound;
class System;
};

// Globals
SoundManager *sound_manager();

// SoundSource class definition.
class SoundSource {
 public:
  SoundSource(): channel_(0) {}
  SoundSource(const SoundSource &rhs): channel_(rhs.channel_) {}

  // A SoundSource has an independent notion of being paused from the SoundManager. It plays only
  // if pause is off both here and on the SoundManager.
  void Play();
  void Pause();
  bool IsPaused() const;

  // Stop() is equivalent to letting the Source run its full duration without looping. At this
  // point, it is effectively gone.
  void Stop();
  bool IsStopped() const;

 private:
  friend class SoundSample;
  SoundSource(FMOD::Channel *channel): channel_(channel) {}
  FMOD::Channel *channel_;
};

// SoundSample class definition
class SoundSample {
 public:
  static SoundSample *Load(BinaryFileReader reader, bool store_compressed = false,
                           float base_volume = 1.0f);
  ~SoundSample();
  SoundSource Play(bool looped = false, bool start_paused = false) const;
 
 private:
  SoundSample(FMOD::Sound *sound, float base_volume)
  : sound_(sound), base_volume_(base_volume) {}
  FMOD::Sound *sound_;
  float base_volume_;
  DISALLOW_EVIL_CONSTRUCTORS(SoundSample);
};

// SoundManager class definition
class SoundManager {
 public:
  bool IsInitialized() const {return system_ != 0;}
  float GetVolume() const {return volume_;}
  void SetVolume(float volume);

  void PlayAllSources();
  void PauseAllSources();
  void StopAllSources();
 private:
  // System interface
  friend class System;
  SoundManager();
  ~SoundManager();
  void Think();

  // SoundSample and SoundSource interface
  friend class SoundSample;
  friend class SoundSource;
  FMOD::System *GetSystem() const {return system_;}

  // Data
  FMOD::System *system_;
  float volume_;
  DISALLOW_EVIL_CONSTRUCTORS(SoundManager);
};

#endif // GLOP_SOUND_H__
