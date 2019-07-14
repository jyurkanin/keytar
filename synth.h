#pragma once

#include <math.h>
#include "controller.h"
#include "operator.h"

#define MAX_NUM_WAVES 8

typedef struct {
  float attack_time;
  float attack_level;
  float decay_time;
  float sustain_level;
  float release_time;
} Envelope;

// a single synth algorithm
// to be used by the audio engine
//Abstract base class
class SynthAlg{
 public:
  int s_func;
  Controller controllers[10];
  SynthAlg(int s) : s_func(s){}
  virtual void setVoice(int n) = 0;
  virtual float tick(float freq, int t, int s, int& state) = 0;
  virtual void getControlMap( char mapping[18][50], int& len, int c_num) = 0; //this is for printing out which knobs/sliders do what in the synth_state menu
  virtual Envelope getEnvelope(int i) = 0;
  void getSynthName(char name[20]);
  virtual int getNumControllers() = 0;
  virtual void setData(unsigned char* data, int len) = 0;
  
  virtual ~SynthAlg(){};
  static const int OSC_ALG = 0;
  static const int SWORD_ALG = 1;
  static const int FM_SIMPLE_ALG = 2;
  static const int FM_THREE_ALG = 3;
  static const int WAVE_TABLE_ALG = 4;
};

class OscAlg : public SynthAlg{
 public:
  Operator vc;
  OscAlg() : SynthAlg(0), vc(controllers[0]){};
  ~OscAlg(){};
  void setVoice(int n);
  float tick(float freq, int t, int s, int& state);
  void getControlMap( char mapping[18][50], int& len, int c_num);
  Envelope getEnvelope(int i);
  void setData(unsigned char* data, int len){}
  int getNumControllers(){return 1;}
};

class SwordAlg : public SynthAlg{
 public:
  Operator op;
  SwordAlg() : SynthAlg(1), op(controllers[0]){};
  ~SwordAlg(){};
  void setVoice(int n);
  float tick(float freq, int t, int s, int& state);
  void getControlMap( char mapping[18][50], int& len, int c_num);
  Envelope getEnvelope(int i);
  void setData(unsigned char* data, int len){}
  int getNumControllers(){return 1;}
};

class FmSimpleAlg : public SynthAlg{
 public:
  Operator carrier;
  Operator modulator;
  
  FmSimpleAlg() : SynthAlg(2), carrier(controllers[0]), modulator(controllers[1]){};
  ~FmSimpleAlg(){};
  void setVoice(int n);
  float tick(float freq, int t, int s, int& state);
  void getControlMap( char mapping[18][50], int& len, int c_num);
  Envelope getEnvelope(int i);
  void setData(unsigned char* data, int len){}
  int getNumControllers(){return 2;}
};

class FmThreeAlg : public SynthAlg{
 public:
  Operator carrier;
  Operator modulator1;
  Operator modulator2;
  
 FmThreeAlg() : SynthAlg(3), carrier(controllers[0]), modulator1(controllers[1]), modulator2(controllers[2]){};
  ~FmThreeAlg(){};
  void setVoice(int n);
  float tick(float freq, int t, int s, int& state);
  void getControlMap( char mapping[18][50], int& len, int c_num);
  Envelope getEnvelope(int i);
  void setData(unsigned char* data, int len){}
  int getNumControllers(){return 3;}
};

class WaveTableAlg : public SynthAlg{
 public:
  WaveTableAlg() : SynthAlg(4){};
  ~WaveTableAlg(){};
  void setVoice(int n);
  float tick(float freq, int t, int s, int& state);
  void getControlMap( char mapping[18][50], int& len, int c_num);
  Envelope getEnvelope(int i);
  void setData(unsigned char* data, int len);
  int getNumControllers(){return 1;}
 private:
  int inter = 1;
  //float *wavetable[MAX_NUM_WAVES]; //maximum of MAX_NUM_WAVES waves in the table
  //int wave_length[MAX_NUM_WAVES];
  //int has_loaded_wave[MAX_NUM_WAVES];
  //int num_waves = 0;
};
