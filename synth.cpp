#include "synth.h"
#include "audio_engine.h"


#define OMEGA (2*M_PI/44100.0) //sample rate adjusted conversion from Hz to rad/s

void SynthAlg::getSynthName(char name[20]){
  switch(s_func){
  case 0:
    strcpy(name,"OSCILLATOR");
    return;
  case 1:
    strcpy(name,"SWORD");
    return;
  case 2:
    strcpy(name, "FM_SIMPLE");
    return;
  case 3:
    strcpy(name, "FM_THREE");
    return;
  case 4:
    strcpy(name, "WAVE_TABLE");
    return;
  }
}


void SwordAlg::setVoice(int n){op.setVoice(0);} //do nothing.

/*
 * At high knob values this kind of sounds like the sword pulling out of a sheath sound effect in movies
 */
float SwordAlg::tick(float freq, int t, int s, int &state){
  float mo;
  float ep = 0;
  if(t < 44100) ep = pow(M_E, -t/4410.0);
  mo = ep*(sin(freq*controllers[0].get_slider(0)*controllers[0].get_knob(0)*.03125*OMEGA*t));
  op.output[0] = sin(freq*t*OMEGA + controllers[0].get_slider(1)*controllers[0].get_knob(1)*.0078125*mo);
  op.envelope(t, s);
  op.filter(t, s);
  return op.getOutput();
}
//mapping will be size 18 and len will say how short it actually is
void SwordAlg::getControlMap(char mapping[18][50], int& len, int c_num){
  if(c_num == 0){
    sprintf(mapping[0], "silder(0)*knob(0)/32 = MOD_FREQ = %f", controllers[0].get_slider(0)*controllers[0].get_knob(0)*.03125);
    sprintf(mapping[1], "silder(1)*knob(1)/128 = LIN_GAIN = %f", controllers[0].get_slider(1)*controllers[0].get_knob(1)*.0078125);
    len = 2;
  }
  else{
    len = 0;
  }
}

Envelope SwordAlg::getEnvelope(int i){
  Envelope e;
  if(i != 0) return e;
  if(i % 2 == 0){
    e.attack_time = controllers[0].get_slider(2) * .0078125; //the actual attack time = (attack_time / 128) * 441000
    e.attack_level = controllers[0].get_slider(5)*.0078125;
    e.decay_time = controllers[0].get_slider(3) * .0078125;
    e.sustain_level = controllers[0].get_slider(6)*.0078125;
    e.release_time = controllers[0].get_slider(4) * .0078125;
  }
  else{
    e.attack_time = controllers[0].get_knob(4) * .0078125; //the actual attack time = (attack_time / 128) * 441000
    e.attack_level = controllers[0].get_knob(7)*.0078125;
    e.decay_time = controllers[0].get_knob(5) * .0078125;
    e.sustain_level = controllers[0].get_knob(8)*.0078125;
    e.release_time = controllers[0].get_knob(6) * .0078125;
  }
  return e;
}







void FmSimpleAlg::setVoice(int n){
  modulator.setVoice(n);
  carrier.setVoice(n);
}

float FmSimpleAlg::tick(float freq, int t, int s, int &state){
  float freq_mod = freq;
  float freq_carrier = freq;
  if(controllers[1].get_button(3)){
    modulator.freq_envelope(t, s, freq_mod);
  }
  modulator.tick(freq_mod, t);
  modulator.envelope(t, s);
  if(!controllers[1].get_button(3)){
    modulator.filter(t, s);
  }
  
  carrier.fm_input = modulator.getOutput();
  if(controllers[0].get_button(3)){
    carrier.freq_envelope(t, s, freq_carrier);
  }
  carrier.tick(freq_carrier, t);
  state = carrier.envelope(t, s);
  if(!controllers[0].get_button(3)){
    carrier.filter(t, s);
  }
  return carrier.getOutput();
}
void FmSimpleAlg::getControlMap(char mapping[18][50], int& len, int c_num){
  if(c_num == 0){
    sprintf(mapping[0], "%s", "Carrier Operator");
    sprintf(mapping[1], "Octave = %d", controllers[0].get_slider(1)/8);
    sprintf(mapping[2], "Detune = %d", controllers[0].get_knob(1));
    sprintf(mapping[3], "Waveform = %d", controllers[0].get_knob(2)/22);
    sprintf(mapping[4], "FM Gain = %f", controllers[0].get_slider(0) * controllers[0].get_knob(0) * .0078125);
    sprintf(mapping[5], "Feedback = %f", controllers[0].get_knob(3)/64.0);
    if(controllers[0].get_button(2))
      sprintf(mapping[6], "%s", "Exponential FM");
    else
      sprintf(mapping[6], "%s", "Linear FM");
    len = 7;
  }
  else if(c_num == 1){
    sprintf(mapping[0], "%s", "Modulator Operator");
    sprintf(mapping[1], "Octave = %d", controllers[1].get_slider(1)/8);
    sprintf(mapping[2], "Detune = %d", controllers[1].get_knob(1));
    sprintf(mapping[3], "Waveform = %d               ", controllers[1].get_knob(2)/22);
    sprintf(mapping[4], "Feedback = %f", controllers[1].get_knob(3)/64.0);
    len = 5;
  }
  else{
    len = 0;
  }
}

Envelope FmSimpleAlg::getEnvelope(int i){
  Envelope e;
  if(i >= 4) return e;

  if(i % 2){
    e.attack_time = controllers[i/2].get_slider(2) * .0078125; //the actual attack time = (attack_time / 128) * 441000
    e.attack_level = controllers[i/2].get_slider(5)*.0078125;
    e.decay_time = controllers[i/2].get_slider(3) * .0078125;
    e.sustain_level = controllers[i/2].get_slider(6)*.0078125;
    e.release_time = controllers[i/2].get_slider(4) * .0078125;
  }
  else{
    e.attack_time = controllers[i/2].get_knob(4) * .0078125; //the actual attack time = (attack_time / 128) * 441000
    e.attack_level = controllers[i/2].get_knob(7)*.0078125;
    e.decay_time = controllers[i/2].get_knob(5) * .0078125;
    e.sustain_level = controllers[i/2].get_knob(8)*.0078125;
    e.release_time = controllers[i/2].get_knob(6) * .0078125;
  }

  return e;
}







void FmThreeAlg::setVoice(int n){
  modulator2.setVoice(n);
  modulator1.setVoice(n);
  carrier.setVoice(n);
}

float FmThreeAlg::tick(float freq, int t, int s, int &state){
  float freq1 = freq;
  float freq2 = freq;
  float freq_c = freq;

  if(controllers[2].get_button(3)){
    modulator2.freq_envelope(t, s, freq2);
  }
  modulator2.tick(freq2, t);
  modulator2.envelope(t, s);
  if(!controllers[2].get_button(3)){
    modulator2.filter(t, s);
  }

  if(controllers[1].get_button(3)){
    modulator1.freq_envelope(t, s, freq1);
  }
  modulator1.fm_input = modulator2.getOutput();
  modulator1.tick(freq1, t);
  modulator1.envelope(t, s);
  if(!controllers[1].get_button(3)){
    modulator1.filter(t, s);
  }

  if(controllers[0].get_button(3)){
    carrier.freq_envelope(t, s, freq_c);
  }
  carrier.fm_input = modulator1.getOutput();
  carrier.tick(freq_c, t);
  state = carrier.envelope(t, s);
  if(controllers[0].get_button(3)){
    carrier.filter(t, s);
  }
  
  return carrier.getOutput();
}
void FmThreeAlg::getControlMap(char mapping[18][50], int& len, int c_num){
  if(c_num == 0){
    sprintf(mapping[0], "%s", "Carrier Operator");
    sprintf(mapping[1], "Octave = %d", controllers[0].get_slider(1)/8);
    sprintf(mapping[2], "Detune = %d", controllers[0].get_knob(1));
    sprintf(mapping[3], "Waveform = %d", controllers[0].get_knob(2)/22);
    sprintf(mapping[4], "FM Gain = %f", controllers[0].get_slider(0) * controllers[0].get_knob(0) * .0078125);
    sprintf(mapping[5], "Feedback = %f", controllers[0].get_knob(3)/64.0);
    if(controllers[0].get_button(2))
      sprintf(mapping[6], "%s", "Exponential FM");
    else
      sprintf(mapping[6], "%s", "Linear FM");
    len = 7;
  }
  else if(c_num == 1){
    sprintf(mapping[0], "%s", "Carrier Operator");
    sprintf(mapping[1], "Octave = %d", controllers[1].get_slider(1)/8);
    sprintf(mapping[2], "Detune = %d", controllers[1].get_knob(1));
    sprintf(mapping[3], "Waveform = %d", controllers[1].get_knob(2)/22);
    sprintf(mapping[4], "FM Gain = %f", controllers[1].get_slider(0) * controllers[1].get_knob(0) * .0078125);
    sprintf(mapping[5], "Feedback = %f", controllers[1].get_knob(3)/64.0);
    if(controllers[1].get_button(2))
      sprintf(mapping[6], "%s", "Exponential FM");
    else
      sprintf(mapping[6], "%s", "Linear FM");
    len = 7;
  }
  else if(c_num == 2){
    sprintf(mapping[0], "%s", "Modulator2 Operator");
    sprintf(mapping[1], "Octave = %d", controllers[2].get_slider(1)/8);
    sprintf(mapping[2], "Detune = %d", controllers[2].get_knob(1));
    sprintf(mapping[3], "Waveform = %d               ", controllers[2].get_knob(2)/22);
    sprintf(mapping[4], "Feedback = %f", controllers[2].get_knob(3)/64.0);
    len = 5;
  }
  else{
    len = 0;
  }
}

Envelope FmThreeAlg::getEnvelope(int i){
  Envelope e;
  if(i >= 6) return e;
  
  if(i % 2){
    e.attack_time = controllers[i/2].get_slider(2) * .0078125; //the actual attack time = (attack_time / 128) * 441000
    e.attack_level = controllers[i/2].get_slider(5)*.0078125;
    e.decay_time = controllers[i/2].get_slider(3) * .0078125;
    e.sustain_level = controllers[i/2].get_slider(6)*.0078125;
    e.release_time = controllers[i/2].get_slider(4) * .0078125;
  }
  else{
    e.attack_time = controllers[i/2].get_knob(4) * .0078125; //the actual attack time = (attack_time / 128) * 441000
    e.attack_level = controllers[i/2].get_knob(7)*.0078125;
    e.decay_time = controllers[i/2].get_knob(5) * .0078125;
    e.sustain_level = controllers[i/2].get_knob(8)*.0078125;
    e.release_time = controllers[i/2].get_knob(6) * .0078125;
  }
  return e;
}








void OscAlg::setVoice(int n){
  vc.setVoice(n);
}

float OscAlg::tick(float freq, int t, int s, int &state){
  //  if(controllers[0].get_button(3)){
  // vc.freq_envelope(t, s, freq);
  //}
  vc.tick(freq, t);
  state = vc.envelope(t, s);
  //if(!controllers[0].get_button(3)){
  //  vc.filter(t, s);
  //}
  return vc.getOutput();
}
void OscAlg::getControlMap(char mapping[18][50], int& len, int c_num){
  if(c_num == 0){
    sprintf(mapping[0], "Octave = %d", (controllers[0].get_slider(1)/8));
    sprintf(mapping[1], "Detune = %f", controllers[0].get_knob(1)/6.35);
    sprintf(mapping[2], "Waveform = %d", controllers[0].get_knob(2)/22);
    sprintf(mapping[3], "Feedback = %f", controllers[0].get_knob(3)/128.0);
    len = 4;
  }
  else{
    len = 0;
  }
}

Envelope OscAlg::getEnvelope(int i){
  Envelope e;
  if( i >= 2) return e;
  if(i % 2){
    e.attack_time = controllers[i/2].get_knob(4) * .0078125; //the actual attack time = (attack_time / 128) * 441000
    e.attack_level = controllers[i/2].get_knob(7)*.0078125;
    e.decay_time = controllers[i/2].get_knob(5) * .0078125;
    e.sustain_level = controllers[i/2].get_knob(8)*.0078125;
    e.release_time = controllers[i/2].get_knob(6) * .0078125;
  }  
  else{
    e.attack_time = controllers[i/2].get_slider(2) * .0078125; //the actual attack time = (attack_time / 128) * 441000
    e.attack_level = controllers[i/2].get_slider(5)*.0078125;
    e.decay_time = controllers[i/2].get_slider(3) * .0078125;
    e.sustain_level = controllers[i/2].get_slider(6)*.0078125;
    e.release_time = controllers[i/2].get_slider(4) * .0078125;
  }
  return e;
}





//todo: pitch shift the cached waveforms. Idk how tho
//new wave table algorithm is going to work by assigning a different
//phase offset to each wave in the table. Then it will do a convolution
//of the wave and an LFO with the phase offset. It will sum the result
//of each convolution for each wave
/*float WaveTableAlg::tick(float freq, int t, int s, int& state){
  float sweep_freq = controllers[0].get_knob(8);
  float osc;
  float output = 0;
  float temp;
  float temp2 = (t/44100.0)*sweep_freq*16; //*32.0 //this is right trust me on this
  int st;
  SynthAlg* temp_synth;
  int num_controllers = getNumAlgorithms() - 1;
  float per = 128.0 / (num_controllers - ((num_controllers - 2) * (controllers[0].get_slider(8)/128.0)));
  
  for(int i = 0; i < 8; i++){
    osc = fmod(temp2 + controllers[0].get_knob(i), 128);
    if(osc < per){
      temp = osc/per;
    }
    else if(osc < (per*2)){
      temp = 2 - (osc/per);
    }
    else{
      temp = 0;
    }

    temp_synth = getSynth(i);
    if(temp_synth != NULL){
      if(temp_synth->s_func != SynthAlg::WAVE_TABLE_ALG){
	output += temp*compute_algorithm(freq, t, s, controllers[0].get_slider(i), i, st);
      }
    }
  }
  
  return output;
}
*/

void WaveTableAlg::setVoice(int n){
  
}

void WaveTableAlg::setData(unsigned char* data, int len){
  
}

void WaveTableAlg::getControlMap(char mapping[18][50], int& len, int c_num){
  if(c_num == 0){
    sprintf(mapping[0], "WAVE_TABLE_SWEEP_FREQ = %d", controllers[0].get_knob(8));
    for(int i = 0; i < 8; i++){
      sprintf(mapping[1+i], "[%d|Pos:%d|Vol:%d]", i, controllers[0].get_knob(i), controllers[0].get_slider(i));
    }
    len = 9;
  }
  else{
    len = 0;
  }
}

Envelope WaveTableAlg::getEnvelope(int i){
  Envelope e;
  return e;
}

float WaveTableAlg::tick(float freq, int t, int s, int& state){
  float step = controllers[0].get_knob(8) * 128.0 / 44100.0f;
  int num_algs = getNumAlgorithms() - 1;
  float curr_index = fmod(step*t, num_algs);
  
  int algs[8];
  int count = 0;
  SynthAlg* temp_synth;
  for(int i = 0; i < 9; i++){
    temp_synth = getSynth(i);
    if(temp_synth != NULL){
      if(temp_synth->s_func != SynthAlg::WAVE_TABLE_ALG){
	algs[count] = i;
      }
    }
  }
  
  float y1;
  float y2;

  int x1 = (int) curr_index;
  int x2 = (x1 + 1) % num_algs;
  
  int st;
  
  y1 = compute_algorithm(freq, t, s, controllers[0].get_slider(x1), x1, st);
  y2 = compute_algorithm(freq, t, s, controllers[0].get_slider(x2), x2, st);

  
  float output = 0;
  int diff1;
  if(curr_index < x1){
    diff1 = curr_index;
  }
  else{
    diff1 = curr_index - x1;
  }

  int diff2;
  if(x2 < curr_index){
    diff2 = x2;
  }
  else{
    diff2 = x2 - curr_index;
  }
  output = y1*(diff1) + y2*(diff2); //yeah that should do it I guess  
  return output;
}
