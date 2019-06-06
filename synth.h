#pragma once

#include <math.h>
#include "controller.h"

#define MAX_NUM_WAVES 8


// a single synth algorithm
// to be used by the audio engine
//Abstract base class
class SynthAlg{
 public:
  int s_func;
  Controller controller;
  SynthAlg(int s) : s_func(s){}
  virtual float tick(float freq, int t) = 0;
  virtual void getControlMap( char**& mapping, int& len) = 0; //this is for printing out which knobs/sliders do what in the synth_state menu
  virtual void getSynthName(char name[20]);
  virtual void setData(unsigned char* data, int len) = 0;
  static const int SIN_ALG = 0;
  static const int SWORD_ALG = 1;
  static const int FM_SIMPLE_ALG = 2;
  static const int WAVE_TABLE_ALG = 3;
};

class SinAlg : public SynthAlg{
 public:
  SinAlg() : SynthAlg(0){};
  float tick(float freq, int t);
  void getControlMap( char**& mapping, int& len);
  void setData(unsigned char* data, int len){}
};

class SwordAlg : public SynthAlg{
 public:
  SwordAlg() : SynthAlg(1){};
  float tick(float freq, int t);
  void getControlMap( char**& mapping, int& len);
  void setData(unsigned char* data, int len){}
};

class FmSimpleAlg : public SynthAlg{
 public:
  FmSimpleAlg() : SynthAlg(2){};
  float tick(float freq, int t);
  void getControlMap( char**& mapping, int& len);
  void setData(unsigned char* data, int len){}
};

class WaveTableAlg : public SynthAlg{
 public:
  WaveTableAlg() : SynthAlg(3){};
  float tick(float freq, int t);
  void getControlMap( char**& mapping, int& len);
  void setData(unsigned char* data, int len);
 private:
  int inter = 1;
  //float *wavetable[MAX_NUM_WAVES]; //maximum of MAX_NUM_WAVES waves in the table
  //int wave_length[MAX_NUM_WAVES];
  //int has_loaded_wave[MAX_NUM_WAVES];
  //int num_waves = 0;
};
