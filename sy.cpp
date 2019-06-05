#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include "piano.h"
#include <string>
#include <iostream>
#include <alsa/asoundlib.h>
#include "wave_window.h"

using namespace stk;
int generate_sample();


int main(int argc, char *argv[]){
    if(argc < 3 ){
      printf("usage ./keyboard <midi keyboard> <midi controller>\n");
      exit(-1);
    }
    //generate_sample();
    init_window();
    init_alsa();
    init_midi(argc, argv);
    while(is_window_open()){
        midi_loop();
    }
    exit_controller();
    exit_alsa();
    exit_midi();
}

float octave_freq[12] = {27.5, 29.135, 30.868, 32.703, 34.648, 36.708, 38.891, 41.203, 43.654, 46.249, 48.999, 51.913}; // frequencies of the lowest octave
int generate_sample(){
    Sample sample;
    Stk::setSampleRate(44100.0);
    SineWave sine;
    StkFrames main_sample(4410, 1);
    double freq;
    
    sine.setFrequency(440.0);    
    sine.tick(main_sample);
    
    for(int k = 21; k <= 108; k++){
        freq = octave_freq[(k-21) % 12] * (1 << (1+(int)(k-21)/12) );
        sample.period[k] = (int) (44100 / freq); 
        sample.notes[k].resize(4410 , 1);
        sine.setFrequency(freq);
        sine.tick(sample.notes[k]);
        //      shifter.setShift(freq/440);
        //shifter.tick(main_sample, sample.notes[k], 1, 1);
    }
    setSample(sample);
    return 1;
}

