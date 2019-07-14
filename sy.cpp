#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include "audio_engine.h"
#include <string>
#include <iostream>
#include <unistd.h>
#include <alsa/asoundlib.h>
#include "wave_window.h"

using namespace stk;
int generate_sample();


int main(int argc, char *argv[]){
  if(argc < 2 ){
    printf("usage ./keyboard <midi keyboard> <midi controller>\n");
    exit(-1);
  }
  //generate_sample();
  init_window();
  init_alsa();
  init_midi(argc, argv);
  while(is_window_open()){
    usleep(10000);
  }
  exit_alsa();
  exit_midi();
}
/*int main(int argc, char *argv[]){
  init_record();
  }*/
