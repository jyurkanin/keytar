#include <math.h>
#include "controller.h"
#include "filter.h"

#define OMEGA (2*M_PI/44100.0) //sample rate adjusted conversion from Hz to rad/s
#define HALF_PI 1.57079632675

#define THREE_HALF_PI 4.71238898025
#define PI_2 6.283185307

//amplitude is 1

float tri(float t);

/*
 * An Operator is a Voltage Controlled Oscillator Paired with two Envelopes
 * One envelope for the filter, one for the amplitude
 * An Operator has an associated Controller. Which controls the oscillator
 * modulation and the envelopes for that oscillator.
 */


class Operator{
 public:
 Operator(Controller &c) : controller(c), lp_filter(22000), r_filter(.5, 22000){};
  void setVoice(int n){voice = n; r_filter.setVoice(n); lp_filter.setVoice(n);}
  float getOutput(){return output[voice];}
  void tick(float freq, int t);
  int freq_envelope(int t, int s, float freq, float &f_mod);
  int envelope(int t, int s);
  int filter(int t, int s);
  static int adsr(float attack_time_norm, float decay_time_norm, float release_time_norm, float attack_level_norm, float sustain_level_norm, int t, int s, float &adsr_out);
  
  Controller &controller;

  RFilter r_filter;
  LPFilter lp_filter;
  
  int voice; //which voice. To prevent interference with the feedback part.
  float output[128];
  float fm_input; //this aliases the output of another operator/function
  float freq_;

  
  const static int IDLE = 1;
  const static int ATTACK = 2;
  const static int DECAY = 3;
  const static int SUSTAIN = 4;
  const static int RELEASE = 5;
};
