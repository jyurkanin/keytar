#include "synth.h"
#include "audio_engine.h"

#define OMEGA (2*M_PI/44100.0) //sample rate adjusted conversion from Hz to rad/s

void SynthAlg::getSynthName(char name[20]){
  switch(s_func){
  case 0:
    strcpy(name,"SIN_WAVE");
    return;
  case 1:
    strcpy(name,"SWORD");
    return;
  case 2:
    strcpy(name, "FM_SIMPLE");
    return;
  case 3:
    strcpy(name, "WAVE_TABLE");
    return;
  }
}


/*
 * At high knob values this kind of sounds like the sword pulling out of a sheath sound effect in movies
 */
float SwordAlg::tick(float freq, int t){
  float mo;
  float ep = 0;
  if(t < 44100) ep = pow(M_E, -t/4410.0);
  mo = ep*(sin(freq*controller.get_slider(0)*controller.get_knob(0)*.03125*OMEGA*t));
  return sin(freq*t*OMEGA + controller.get_slider(1)*controller.get_knob(1)*.0078125*mo);
}
//mapping will be size 18 and len will say how short it actually is
void SwordAlg::getControlMap(char**& mapping, int& len){
  sprintf(mapping[0], "silder(0)*knob(0)/32 = MOD_FREQ = %f", controller.get_slider(0)*controller.get_knob(0)*.03125);
  sprintf(mapping[1], "silder(1)*knob(1)/128 = LIN_GAIN = %f", controller.get_slider(1)*controller.get_knob(1)*.0078125);
  len = 2;
}


float FmSimpleAlg::tick(float freq, int t){
  float mod_freq = (controller.get_slider(0)/4) + (controller.get_knob(0)/128.0);
  float mo = sin(freq*t*OMEGA*mod_freq);
  return sin(freq*t*OMEGA + controller.get_slider(1)*controller.get_knob(1)*mo*.0078125);
}
void FmSimpleAlg::getControlMap(char**& mapping, int& len){
  float mod_freq = (controller.get_slider(0)/4) + (controller.get_knob(0)/128.0);
  sprintf(mapping[0], "MOD_FREQ = %f", mod_freq);
  sprintf(mapping[1], "LIN_GAIN = %f", controller.get_slider(1)*controller.get_knob(1)/128.0);
  len = 2;
}

float SinAlg::tick(float freq, int t){
  float mod_freq = (controller.get_slider(0)/4) + (controller.get_knob(0)/128.0);
  return sin(t*OMEGA*freq*mod_freq);
}
void SinAlg::getControlMap(char**& mapping, int& len){
  float mod_freq = (controller.get_slider(0)/4) + (controller.get_knob(0)/128.0);
  sprintf(mapping[0], "Octave = %d", controller.get_slider(0)/4);
  sprintf(mapping[1], "Detune = %f", controller.get_knob(0)/128.0);
  len = 2;
}


//todo: pitch shift the cached waveforms. Idk how tho
//new wave table algorithm is going to work by assigning a different
//phase offset to each wave in the table. Then it will do a convolution
//of the wave and an LFO with the phase offset. It will sum the result
//of each convolution for each wave
float WaveTableAlg::tick(float freq, int t){
  float sweep_freq = controller.get_knob(8);
  float osc;
  float output = 0;
  float temp;
  float temp2 = (t/44100)*sweep_freq*32; //this is right trust me on this
  
  for(int i = 0; i < 8; i++){
    osc = fmod(temp2 + controller.get_knob(i), 128);
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
  sprintf(mapping[0], "knob(8)/128 = WAVE_TABLE_SWEEP_FREQ = %f", controller.get_knob(8)/4.0);
  sprintf(mapping[1], "silder(8) = ? = %d", controller.get_slider(8));
  for(int i = 0; i < 8; i++){
    sprintf(mapping[2+i], "[%d|Pos:%d|Vol:%d]", i, controller.get_knob(i), controller.get_slider(i));
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
  float step = controller.get_knob(8) / 128.0f;
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
