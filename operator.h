#include <math.h>
#include "controller.h"

#define OMEGA (2*M_PI/44100.0) //sample rate adjusted conversion from Hz to rad/s
#define HALF_PI 1.57079632675

#define THREE_HALF_PI 4.71238898025
#define PI_2 6.283185307

//amplitude is 1

float tri(float t);

/*
 * An Operator is a Voltage Controlled Oscillator Paired with an Envelope
 * An Operator has an associated Controller. Which controls the oscillator
 * modulation and the envelope for that oscillator.
 */


class Operator{
 public:
  Operator(Controller &c) : controller(c){};
  void tick(float freq, int t);
  int envelope(int t, int s);
  
  Controller &controller;
  float output;
  float fm_input; //this aliases the output of another operator/function
  float freq_;
  
  const static int IDLE = 1;
  const static int ATTACK = 2;
  const static int DECAY = 3;
  const static int SUSTAIN = 4;
  const static int RELEASE = 5;
};
