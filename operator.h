#include <math.h>

#define OMEGA (2*M_PI/44100.0) //sample rate adjusted conversion from Hz to rad/s
#define HALF_PI 1.57079632675
#define PI 3.1415926535
#define THREE_HALF_PI 4.71238898025
#define PI_2 6.283185307

//amplitude is 1
//when tri(0) -> 0
//when tri(pi/2) -> 1
//when tri(pi) -> 0
//when tri(3*pi/2) -> -1
//when tri(2*pi) -> 0

float tri(float t);


class Operator{
 public:
  Operator(int &octave_, int &detune_, int &waveform_, float &fm_mod_, int &beta_1_, int &beta_2_) : octave(octave_), detune(detune_), waveform(waveform_), fm_mod(fm_mod_), beta_1(beta_1_), beta_2(beta_2_), state(IDLE){}
  void tick(float freq, int t, int s);
  int envelope(int t);
  
  float output;

  int &octave; //these alias controller values.
  int &detune; //this too.
  int &waveform; //this too
  float &fm_mod; //this aliases the output of another operator/function
  
  int &beta_1; //controller alias
  int &beta_2; //controller alias

  //envelope values
  int &attack_level; //controller alias
  int &attack_time; //need to be normalized. Divided by 128
  int &decay_time;
  int &sustain_level;
  int &release_time;

  const int IDLE = 1;
  const int ATTACK = 2;
  const int DECAY = 3;
  const int SUSTAIN = 4;
  const int RELEASE = 5;
}


/*
  
 private:
  int state;
*/
