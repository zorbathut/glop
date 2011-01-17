// Load and play sounds.

#ifndef GLOP_SOUND_H__
#define GLOP_SOUND_H__

#ifndef GLOP_LEAN_AND_MEAN

#ifdef IPHONE // TODO: remove this horrible horrible hack
#include "Sound_Iphone.h"
#else

// Includes
#include "Base.h"
#include "Stream.h"

// Class declarations
class SoundManager;
struct FMOD_CHANNEL;
struct FMOD_SOUND;
struct FMOD_SYSTEM;

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
  SoundSource(FMOD_CHANNEL *channel): channel_(channel) {}
  FMOD_CHANNEL *channel_;
};

// SoundSample class definition
class SoundSample {
 public:
  static SoundSample *Load(InputStream input, bool store_compressed = false,
                           float base_volume = 1.0f);
  ~SoundSample();
  SoundSource Play(bool looped = false, bool start_paused = false) const;
 
 private:
  SoundSample(FMOD_SOUND *sound, float base_volume)
  : sound_(sound), base_volume_(base_volume) {}
  FMOD_SOUND *sound_;
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
  FMOD_SYSTEM *GetSystem() const {return system_;}

  // Data
  FMOD_SYSTEM *system_;
  float volume_;
  DISALLOW_EVIL_CONSTRUCTORS(SoundManager);
};

#endif // horrible horrible hackery

#endif // GLOP_LEAN_AND_MEAN

#endif // GLOP_SOUND_H__
