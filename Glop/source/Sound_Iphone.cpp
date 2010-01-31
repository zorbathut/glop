
#include "Sound.h"

SoundManager::SoundManager() { };
SoundManager::~SoundManager() { };

void SoundManager::Think() { };


SoundSample *SoundSample::Load(InputStream input, bool store_compressed, float base_volume) {
  return NULL;
};
SoundSource SoundSample::Play(bool looped, bool start_paused) const {
  return SoundSource();
};
SoundSample::~SoundSample() {
};


bool SoundSource::IsStopped() const {
  return true;
}
void SoundSource::Stop() { }
