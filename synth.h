#include <math.h>
#include "piano.h"

#ifndef __SYNTH_H__
#define __SYNTH_H__

#define MAX_NUM_WAVES 8


// a single synth algorithm
// to be used by the audio engine
//Abstract base class
class SynthAlg{
  int s_func;
  Controller controller;
  SynthAlg(int s) s_func(s){}
  virtual float tick(float freq, int t) = 0;
  virtual void getControlMap(char**& mapping, int& len) = 0; //this is for printing out which knobs/sliders do what in the synth_state menu
  virtual void setData(unsigned char* data, int len) = 0;
  static const SIN_ALG = 0;
  static const SWORD_ALG = 1;
  static const FM_SIMPLE_ALG = 2;
  static const WAVE_TABLE_ALG = 3;
};

class SinAlg : SynthAlg{
 SinAlg() : SynthAlg(0){};
  float tick(float freq, int t);
  void getControlMap(char**& mapping, int& len);
  void setData(unsigned char* data, int len){}
}

class SwordAlg : SynthAlg{
 SwordAlg() : SynthAlg(1){};
  float tick(float freq, int t);
  void getControlMap(char**& mapping, int& len);
  void setData(unsigned char* data, int len){}
}

class FmSimpleAlg : SynthAlg{
 FmSimpleAlg() : SynthAlg(2){};
  float tick(float freq, int t);
  void getControlMap(char**& mapping, int& len);
  void setData(unsigned char* data, int len){}
}

class WaveTableAlg : SynthAlg{
 WaveTableAlg() : SynthAlg(3){};
  float tick(float freq, int t);
  void getControlMap(char**& mapping, int& len);
  void setData(unsigned char* data, int len);
 private:
  int inter = 1;
  //float *wavetable[MAX_NUM_WAVES]; //maximum of MAX_NUM_WAVES waves in the table
  //int wave_length[MAX_NUM_WAVES];
  //int has_loaded_wave[MAX_NUM_WAVES];
  //int num_waves = 0;
}


#endif
