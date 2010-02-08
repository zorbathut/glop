#ifndef GLOP_SOUND_IPHONE_H__
#define GLOP_SOUND_IPHONE_H__

#include "Base.h"
#include "Stream.h"

// Class declarations
class SoundManager;

// Globals
SoundManager *sound_manager();

// SoundSource class definition.
class SoundSource {
 public:
  SoundSource() {}

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
  void *avap;
};

// SoundSample class definition
class SoundSample {
 public:
  static SoundSample *Load(InputStream input, bool store_compressed = false,
                           float base_volume = 1.0f);
  ~SoundSample();
  SoundSource Play(bool looped = false, bool start_paused = false) const;
 
 private:
  SoundSample() {};

  char *data_;
  int data_len_;
  float base_volume_;
  DISALLOW_EVIL_CONSTRUCTORS(SoundSample);
};

// SoundManager class definition
class SoundManager {
 public:
  //bool IsInitialized() const {return system_ != 0;}
  
  /*
  float GetVolume() const {return volume_;}
  void SetVolume(float volume);

  void PlayAllSources();
  void PauseAllSources();
  void StopAllSources();*/ // no likey
  
 private:
  // System interface
  friend class System;
  SoundManager();
  ~SoundManager();
  void Think();

 /*
  // SoundSample and SoundSource interface
  friend class SoundSample;
  friend class SoundSource;
  FMOD_SYSTEM *GetSystem() const {return system_;}

  // Data
  FMOD_SYSTEM *system_;
  float volume_;*/
  DISALLOW_EVIL_CONSTRUCTORS(SoundManager);
};

#endif
