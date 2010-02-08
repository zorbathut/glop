
#include "Sound.h"

#include "Base.h"

#import <AVFoundation/AVAudioPlayer.h>
#import <Foundation/NSData.h>

// yupyupyupyupyupyupyup
SoundManager::SoundManager() { };
SoundManager::~SoundManager() { };

// uh-huh, uh-huh
void SoundManager::Think() { };


SoundSample *SoundSample::Load(InputStream in, bool store_compressed, float base_volume) {
  if (!in.IsValid())
    return 0;
  
  SoundSample *samp = new SoundSample;;
  samp->data_len_ = in.ReadAllData((void**)&samp->data_);
  samp->base_volume_ = base_volume;
  
  return samp;
};
SoundSource SoundSample::Play(bool looped, bool start_paused) const {
  SoundSource src;
  NSError *err;
  AVAudioPlayer *avas = [[AVAudioPlayer alloc] initWithData:[NSData dataWithBytesNoCopy:data_ length:data_len_ freeWhenDone:NO] error:&err];
  ASSERT(err == nil);
  ASSERT(!start_paused);
  
  if(looped)
    [avas setNumberOfLoops:-1];
  
  [avas setVolume:base_volume_];
  
  [avas play];
  
  src.avap = (void*)avas;
  return src;
};
SoundSample::~SoundSample() {
  delete [] data_; // what happens if we're still playing this sound?
};


bool SoundSource::IsStopped() const {
  return [(AVAudioPlayer*)avap playing];
}
void SoundSource::Stop() {
 [(AVAudioPlayer*)avap stop];
 [(AVAudioPlayer*)avap release];
 avap = nil;
}
