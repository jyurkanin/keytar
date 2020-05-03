#include "operator.h"
#include <stdlib.h>

float tri(float t){
  float temp = fmod(t, PI_2);
  if(temp < HALF_PI){
    return temp / HALF_PI;
  }
  else if(temp < THREE_HALF_PI){
    return (M_PI - temp) / HALF_PI;
  }
  else if(temp < PI_2){
    return (temp - PI_2) / HALF_PI;
  }
  return -2; //this will never happen chill out.
}

float saw(float t){
  return (fmod(t, PI_2) - M_PI) / M_PI;
}

float square(float t){
  return 1 - ((int) (fmod(t, PI_2)/M_PI))*2; //ranges from 0-1
}

//t is the total time on.
//s is the time since it was released
int Operator::adsr(float attack_time_norm, float decay_time_norm, float release_time_norm, float attack_level_norm, float sustain_level_norm, int t, int s, float &adsr_out){  
  //this is going to assume that the note is on.
  if(t < attack_time_norm){
    adsr_out = (t/attack_time_norm)*attack_level_norm;
    return ATTACK;
  }
  else if(t < (attack_time_norm + decay_time_norm)){
    adsr_out = attack_level_norm - (((t - attack_time_norm)/decay_time_norm)*(attack_level_norm - sustain_level_norm));
    return DECAY;
  }
  else if(s == 0){
    adsr_out = sustain_level_norm;
    return SUSTAIN;
  }
  else if(s < release_time_norm){
    adsr_out = sustain_level_norm * (1 - ((float)s / release_time_norm));
    return RELEASE;
  }
  else{
    adsr_out = 0;
    return IDLE;
  }
}

void Operator::tick(float freq, int t){
  float fm_mod;
  int octave = controller.get_slider(1) / 8; //gives a range of 16 octaves
  float detune  = controller.get_knob(1) / 128.0;
  float feedback = controller.get_knob(3) / 64.0;
  
  freq *= (1 << octave) / 128.0;
  freq *= (1+(detune/128.0)); //detune now ranges from 0 - 127 hz
  
  if(controller.get_button(2)){ //button zero is exponential fm mod. Vs linear fm mod.
    fm_mod = controller.get_slider(0)*controller.get_knob(0)*fm_input*.0078125;
    fm_mod += feedback*output[voice];
    switch(controller.get_knob(2)/22){
    case 0:
      output[voice] = sin(OMEGA*t*freq*fm_mod);
      break;
    case 1:
      output[voice] = abs(2*sin(OMEGA*t*freq*.5*fm_mod)) - 1;
      break;
    case 2:
      output[voice] = tri(OMEGA*t*freq*fm_mod);
      break;
    case 3:
      output[voice] = saw(OMEGA*t*freq*fm_mod);
      break;
    case 4:
      output[voice] = square(OMEGA*t*freq*fm_mod);
      break;
    case 5:
      output[voice] = (2.0*((float)rand()/(float)RAND_MAX)) - 1;
      break;
    }
  }
  else{
    fm_mod = controller.get_slider(0)*controller.get_knob(0)*fm_input*.0078125;
    fm_mod += feedback*output[voice];
    switch(controller.get_knob(2)/22){
    case 0:
      output[voice] = sin(OMEGA*t*freq + fm_mod);
      break;
    case 1:
      output[voice] = abs(2*sin(OMEGA*t*freq*.5 + fm_mod)) - 1;
      break;
    case 2:
      output[voice] = tri(OMEGA*t*freq + fm_mod);
      break;
    case 3:
      output[voice] = saw(OMEGA*t*freq + fm_mod);
      break;
    case 4:
      output[voice] = square(OMEGA*t*freq + fm_mod);
      break;
    case 5:
      output[voice] = (2.0*((float)rand()/(float)RAND_MAX)) - 1;
      break;
    }
  }
  
  freq_ = freq;
}

//t is the total time on.
//s is the time since it was released
int Operator::envelope(int t, int s){
  float attack_time_norm = controller.get_slider(2) * .0078125 * 44100.0; //the actual attack time = (attack_time / 128) * 441000
  float decay_time_norm = controller.get_slider(3) * .0078125 * 44100.0;
  float release_time_norm = controller.get_slider(4) * .0078125 * 44100.0;
  
  float attack_level_norm = controller.get_slider(5)*.0078125;
  float sustain_level_norm = controller.get_slider(6)*.0078125;

  float adsr_out;
  int state = adsr(attack_time_norm, decay_time_norm, release_time_norm, attack_level_norm, sustain_level_norm, t, s, adsr_out);
  
  output[voice] *= adsr_out;
  return state;
}




int Operator::freq_envelope(int t, int s, float freq, float &fm_mod){
  static float integral[128];
  float attack_time_norm = controller.get_knob(4) * .0078125 * 44100.0; //the actual attack time = (attack_time / 128) * 441000
  float decay_time_norm = controller.get_knob(5) * .0078125 * 44100.0;
  float release_time_norm = controller.get_knob(6) * .0078125 * 44100.0;
  
  float attack_level_norm = controller.get_knob(7)*.0078125;
  float sustain_level_norm = controller.get_knob(8)*.0078125;

  float adsr_out;
  int state = adsr(attack_time_norm, decay_time_norm, release_time_norm, attack_level_norm, sustain_level_norm, t, s, adsr_out);

  if(t == 0) integral[voice] = 0;
  integral[voice] += (.01*log(freq)*adsr_out);
  
  fm_mod = integral[voice];
  return state;
}

//t is the total time on.
//s is the time since it was released
int Operator::filter(int t, int s){
  float attack_time_norm = controller.get_knob(4) * .0078125 * 44100.0; //the actual attack time = (attack_time / 128) * 441000
  float decay_time_norm = controller.get_knob(5) * .0078125 * 44100.0;
  float release_time_norm = controller.get_knob(6) * .0078125 * 44100.0;
  
  float attack_level_norm = controller.get_knob(7)*.0078125* (20000-freq_);
  float sustain_level_norm = controller.get_knob(8)*.0078125* (20000-freq_);
  
  float r_cutoff;
  float lp_cutoff;

  float adsr_out;
  int state = adsr(attack_time_norm, decay_time_norm, release_time_norm, attack_level_norm, sustain_level_norm, t, s, adsr_out);
  r_cutoff = lp_cutoff = adsr_out;
  
  if(controller.get_button(0)){
    r_filter.setQFactor(controller.get_slider(7)/128.0);
    r_filter.setCutoff(r_cutoff);
  }
  if(controller.get_button(1)){
    lp_filter.setQFactor((1 + controller.get_slider(8))/12.8);
    lp_filter.setCutoff(lp_cutoff);
  }
  
  if(controller.get_button(0) && controller.get_button(1)){
    output[voice] = r_filter.tick(output[voice]) + lp_filter.tick(output[voice]);
  }
  else if(controller.get_button(0)){
    output[voice] = r_filter.tick(output[voice]);
  }
  else if(controller.get_button(1)){
    output[voice] = lp_filter.tick(output[voice]);
  }
  return state;
}
