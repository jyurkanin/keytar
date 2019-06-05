#include "synth.h"

#define OMEGA (2*M_PI/44100.0) //sample rate adjusted conversion from Hz to rad/s

char *SynthAlg::getSynthName(){
  switch(s_func){
  case 0:
    return "SIN_WAVE";
  case 1:
    return "SWORD";
  case 2:
    return "FM_SIMPLE";
  case 3:
    return "WAVE_TABLE";
  }
}


/*
 * At high knob values this kind of sounds like the sword pulling out of a sheath sound effect in movies
 */
float SwordAlg::tick(float freq, int t){
  float mo;
  float ep = 0;
  if(t < 44100) ep = pow(M_E, -t/4410.0);
  mo = ep*(sin(freq*get_slider(0)*get_knob(0)*.03125*OMEGA*t));
  return sin(freq*t*OMEGA + get_slider(1)*get_knob(1)*.0078125*mo);
}
//mapping will be size 18 and len will say how short it actually is
void SwordAlg::getControlMap(char**& mapping, int& len){
  sprintf(mapping[0], "silder(0)*knob(0)/32 = MOD_FREQ = %f", get_slider(0)*get_knob(0)/32.0);
  sprintf(mapping[1], "silder(1)*knob(1)/128 = LIN_GAIN = %f", get_slider(1)*get_knob(1)/128.0);
  len = 2;
}


float FmSimpleAlg::tick(float freq, int t){
  float mo = sin((freq*t*OMEGA) + (get_slider(0)*get_knob(0)*.03125));
  return sin(freq*t*OMEGA + get_slider(1)*get_knob(1)*mo*.0078125);
}
void FmSimpleAlg::getControlMap(char**& mapping, int& len){
  sprintf(mapping[0], "silder(0)*knob(0)*.1 = MOD_FREQ = %f", get_slider(0)*get_knob(0)/32.0);
  sprintf(mapping[1], "silder(1)*knob(1)*.1 = LIN_GAIN = %f", get_slider(1)*get_knob(1)/128.0);
  len = 2;
}

float SinAlg::tick(float freq, int t){
  return sin(freq*t*OMEGA + (get_slider(0)*get_knob(0)*.03125));
}
void SinAlg::getControlMap(char**& mapping, int& len){
  sprintf(mapping[0], "silder(0)*knob(0)*.1 = MOD_FREQ = %f", get_slider(0)*get_knob(0)/32.0);
  len = 1;
}


//todo: pitch shift the cached waveforms. Idk how tho
//new wave table algorithm is going to work by assigning a different
//phase offset to each wave in the table. Then it will do a convolution
//of the wave and an LFO with the phase offset. It will sum the result
//of each convolution for each wave
float WaveTableAlg::tick(float freq, int t){
  float sweep_freq = get_knob(8);
  float osc;
  float output = 0;
  float temp;
  float temp2 = (t/44100)*sweep_freq*32; //this is right trust me on this
  
  for(int i = 0; i < 8; i++){
    osc = fmod(temp2 + get_knob(i), 128);
    if(osc < 8){
      temp = osc*.125;
    }
    else if(osc < 16){
      temp = 2 - (osc*.125);
    }
    else{
      temp = 0;
    }
    output += temp*compute_algorithm(freq, t, controller.get_slider(i), i);
  }
  
  //maybe do cubic interpolation one day. Idk
  return output;
}

void WaveTableAlg::setData(unsigned char* data, int len){
  
}

void WaveTableAlg::getControlMap(char**& mapping, int& len){
  sprintf(mapping[0], "knob(8)/128 = WAVE_TABLE_SWEEP_FREQ = %f", get_knob(8));
  sprintf(mapping[1], "silder(8) = ? = %f", get_slider(8));
  for(int i = 0; i < 8; i++){
    sprintf(mapping[2+i], "[%d|Pos:%d|Vol:%d]", get_knob(i), get_slider(i));
  }
  len = 10;
}




/*float compute_sample2(int n, int t){
    float freq = freqs[(n-21) % 12] * (1 << (1+(int)(n-21)/12) );
    float mo;
    float omega = 2*M_PI/44100.0;
    float ep = 0;    
    if(t < 44100) ep = pow(M_E, -t/4410.0);
    mo = ep*(sin(freq*25*omega*t));
    return sin(freq*t*omega + 1000*mo);
}*/

/*
float WaveTableAlg::tick(float freq, int t){
  float step = get_knob(8) / 128.0f;
  float real_index = fmod(step*t, 128);
  
  float x1 = (int) real_index; //real_index floored,
  float x2 = (x1+1) % 128;

  
  float y1;
  float y2;

  //if you haven't pressed the button that caches the waveforms, then dynamically compute them.
  //  if(has_loaded_wave[x1]) y1 = wavetable[x1][t%wave_length[x1]];
  //else
  y1 = compute_algorithm(n, t, controller.get_slider(x1), x1);
  
  //if(has_loaded_wave[x2]) y2 = wavetable[x2][t%wave_length[x2]];
  //else
  y2 = compute_algorithm(n, t, controller.get_slider(x2), x2);
  
  float output = 0;
  switch(inter){
  case 0: //no interpolation. Floor real_index //dont do this
    output = y1;
    break;
  case 1:
    output = y1*(real_index-x1) + y2*(x2-real_index); //yeah that should do it I guess  
    break;
  }
  
  //maybe do cubic interpolation one day. Idk
  return output;
  }*/
