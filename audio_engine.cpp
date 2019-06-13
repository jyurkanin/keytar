#include "audio_engine.h"
#include <fstream>
#include <iostream>
#include <fcntl.h>
#include <vector>
#include <queue>
#include <sys/ioctl.h>

ADSR_S adsr[128];
snd_pcm_t *playback_handle;
pthread_t m_thread;
pthread_t piano_midi_thread;
int MidiFD;
MidiByte midiNotesPressed[0xFF]; /* this records all the notes currently on by pitch maximum notes on is KEYS*/
MidiByte midiNotesSustained[0xFF];
float freqs[12] = {27.5, 29.135, 30.868, 32.703, 34.648, 36.708, 38.891, 41.203, 43.654, 46.249, 48.999, 51.913}; // frequencies of the lowest octave
std::vector<SynthAlg*> synth_algorithms;
//SynthAlg *synth_algorithms[MAX_NUM_WAVES];
//int num_algorithms = 0;
Controller main_controller;
Sample sample;


void breakOnMe(){
    //break me on, Break on meeeeeee
}

void save_program(char* filename){
  char file_path[110];
  sprintf(file_path, "programs/%s", filename);
  std::ofstream out;
  out.open(file_path, std::ofstream::out);
  
  out << synth_algorithms.size() << "\n";
  
  for(int i = 0; i < synth_algorithms.size(); i++){
    out << synth_algorithms[i]->s_func << "\n";
  }

  unsigned char slider[9];
  unsigned char knob[9];
  
  memcpy(slider, main_controller.slider, sizeof(slider));
  memcpy(knob, main_controller.knob, sizeof(knob));
  out << slider[0] << " " << slider[1] << " " <<  slider[2] << " " << slider[3] << " " << slider[4] << " " << slider[5] << " " << slider[6] << " " << slider[7] << " " << slider[8] << " ";
  out << knob[0] << " " << knob[1] << " " <<  knob[2] << " " << knob[3] << " " << knob[4] << " " << knob[5] << " " << knob[6] << " " << knob[7] << " " << knob[8] << "\n";
  
  for(int i = 0; i < synth_algorithms.size(); i++){
    memcpy(slider, synth_algorithms[i]->controller.slider, sizeof(slider));
    memcpy(knob, synth_algorithms[i]->controller.knob, sizeof(knob));
    out << slider[0] << " " << slider[1] << " " <<  slider[2] << " " << slider[3] << " " << slider[4] << " " << slider[5] << " " << slider[6] << " " << slider[7] << " " << slider[8] << " ";
    out << knob[0] << " " << knob[1] << " " <<  knob[2] << " " << knob[3] << " " << knob[4] << " " << knob[5] << " " << knob[6] << " " << knob[7] << " " << knob[8] << "\n";
  }

  out.close();
}

void load_program(char* filename){
  for(int i = 0; i < synth_algorithms.size(); i++){
    delete synth_algorithms[i];
  }  
  synth_algorithms.clear();
  
  std::ifstream in;
  char file_path[110];
  sprintf(file_path, "programs/%s", filename);
  in.open(file_path, std::ifstream::in);
  int num;
  in >> num; //whitespace delimited

  int temp;
  for(int i = 0; i < num; i++){
    in >> temp;
    addSynth(temp);
  }

  for(int j = 0; j < 9; j++){
    in >> main_controller.slider[j];
  }
  for(int j = 0; j < 9; j++){
    in >> main_controller.knob[j];
  }
  
  for(int i = 0; i < num; i++){
    for(int j = 0; j < 9; j++){
      in >> synth_algorithms[i]->controller.slider[j];
    }
    for(int j = 0; j < 9; j++){
      in >> synth_algorithms[i]->controller.knob[j];
    }
  }
  
}

int getNumAlgorithms(){
  return synth_algorithms.size();
}

SynthAlg* getSynth(int num){
  if(num < synth_algorithms.size())
    return synth_algorithms[num];
  return NULL;
}

Controller* getMainController(){
  return &main_controller;
}

void activate_main_controller(){
  main_controller.activate();
}

//freq normalized from 1-0
float low_pass(float input, float freq){
  static float  output = 0;
  output += freq*(input-output);
  return output;
}

float synthesize(int n, int t, int volume){
  float sample = 0;
  
  for(int i = 0; i < synth_algorithms.size(); i++){
    sample += main_controller.get_slider(i) * compute_algorithm(n, t, volume, i) / 128.0f;
  }
  
  return sample;
}

float compute_algorithm(int n, int t, int volume, int alg_num){
  float freq = freqs[(n-21) % 12] * (1 << (1+(int)(n-21)/12));
  return compute_algorithm(freq, t, volume, alg_num);
}
float compute_algorithm(float freq, int t, int volume, int alg_num){
  if(alg_num >= synth_algorithms.size()) return 0;
  SynthAlg *synth = synth_algorithms[alg_num];
  //todo: implement some of the yamaha dx11 FM algorithms.
  return synth->tick(freq, t) * volume / 128.0f; //sexy as hell. Thanks c++
}

void addSynth(int alg){
  if(synth_algorithms.size() >= MAX_NUM_WAVES) return;
  switch(alg){
  case SynthAlg::SIN_ALG:
    synth_algorithms.push_back(new SinAlg());
    break;
  case SynthAlg::SWORD_ALG:
    synth_algorithms.push_back(new SwordAlg());
    break;
  case SynthAlg::FM_SIMPLE_ALG:
    synth_algorithms.push_back(new FmSimpleAlg());
    break;
  case SynthAlg::WAVE_TABLE_ALG:
    synth_algorithms.push_back(new WaveTableAlg());
    break;
  }  
}

void delSynth(int alg){
  if(alg >= synth_algorithms.size()) return;
  SynthAlg* temp = synth_algorithms[alg];
  synth_algorithms.erase(synth_algorithms.begin()+alg);
  delete temp;
}

void *audio_thread(void *arg){
    stk::StkFrames temp;
    stk::StkFrames temp_frames(441, 1);
    stk::StkFrames looped_frames_window(441, 1);
    float sum_frames[441*CHANNELS];
    stk::StkFrames in_frames(44100, 1);
    stk::StkFrames out_frames(44100, 1);
    int err;
    int num_pressed;
    int frames_to_deliver;
    int volume = 0;
    int ip_algo = 0; //interpolation algorithm
    
    snd_pcm_state_t pcm_state;
    int lowest_note;
    int lowest_index;
    
    while(is_window_open()){
        snd_pcm_wait(playback_handle, 100);
        frames_to_deliver = snd_pcm_avail_update(playback_handle);
        if ( frames_to_deliver == -EPIPE){
	    snd_pcm_prepare(playback_handle);
            printf("Epipe\n");
            continue;
        }
        frames_to_deliver = frames_to_deliver > 441 ? 441 : frames_to_deliver;
	if(frames_to_deliver == 0){
	  printf("zeruh\n");
	  continue;
	}
	
        memset(sum_frames, 0, CHANNELS*441*sizeof(float));
        num_pressed = 0;
	lowest_note = 0;
        for(int k = 21; k <= 108; k++){
            if(midiNotesPressed[k] || midiNotesSustained[k]){
                num_pressed++;
		sample.volume[k] = midiNotesSustained[k] + midiNotesPressed[k];
		
                if(adsr[k].getState() == stk::ADSR::RELEASE || adsr[k].getState() == stk::ADSR::IDLE){ //detect keyOn
		    printf("Key On\n");
                    adsr[k].keyOn();
                    sample.index[k] = 0;
                }

		if(!lowest_note){
		  lowest_note = k;
		  lowest_index = sample.index[k];
		}
                for(int j = 0; j < frames_to_deliver; j++){
		  sum_frames[j] += adsr[k].tick(synthesize(k, sample.index[k], sample.volume[k]));
		  sample.index[k]++;
                }
            }
            else if(adsr[k].getState() != stk::ADSR::IDLE){ //note was released. Release.
                num_pressed++;
                if(adsr[k].getState() != stk::ADSR::RELEASE){ //detect keyOff
		    printf("Key OFF\n");
                    adsr[k].keyOff();
                }
		
		if(!lowest_note){
		  lowest_note = k;
		  lowest_index = sample.index[k];
		}
                for(int j = 0; j < frames_to_deliver; j++){
		    sum_frames[j] += adsr[k].tick(synthesize(k, sample.index[k], sample.volume[k]));
                    sample.index[k]++;
                }
		
                if(adsr[k].getState() == stk::ADSR::IDLE){
                    sample.index[k] = 0;
                }
            }
        }
	
	//	for(int j = 0; j < frames_to_deliver; j++){
	//  sum_frames[j] = main_controller.get_knob(0)/128.0;//low_pass(sum_frames[j], main_controller.get_knob(0)/128.0);
	//}

	//ODDLY it reduces computational load a shit ton when this is allwed to run all the time. IDK bruh
        if(num_pressed){
	    set_wave_buffer(lowest_note, lowest_index, frames_to_deliver, sum_frames);
	
	    while((err = snd_pcm_writei (playback_handle, sum_frames, frames_to_deliver)) != frames_to_deliver && is_window_open()) {
	      snd_pcm_prepare (playback_handle);
	      pcm_state = snd_pcm_state(playback_handle);
	      fprintf (stderr, "write to audio interface failed (%s)\n", snd_strerror (err));
	    }
	}
	else{
	  usleep(100);
	}
	
	//else{
	//    snd_pcm_prepare (playback_handle); //Everytime I use prepare it leaks a shit ton of memory.
	//}
    }
    printf("Audio Thread is DEADBEEF\n");
}

int init_midi(int argc, char *argv[]){
  char *MIDI_DEVICE = argv[1];
  int flags;
  MidiFD = open(MIDI_DEVICE, O_RDONLY);
  
  flags = fcntl(MidiFD, F_GETFL, 0);
  if(fcntl(MidiFD, F_SETFL, flags | O_NONBLOCK)){
    printf("A real bad ERror %d\n", errno);
  }
  
  if(MidiFD < 0){
    printf("Error: Could not open device %s\n", MIDI_DEVICE);
    exit(-1);
  }
    
//    snd_pipe.openFile("snd_pipe", 1, stk::FileWrite::FILE_SND, stk::Stk::STK_SINT16);
    stk::OneZero attack(0);
    stk::OneZero decay(0);
    stk::OneZero sustain(0);
    stk::OneZero release(0);
    
    for(int k = 21; k <= 108; k++){
        adsr[k] = ADSR_S();
        adsr[k].setAllTimes(.1, .1, 1, .1);
        adsr[k].set_filters(attack, decay, sustain, release);
    }

    pthread_create(&piano_midi_thread, NULL, &midi_loop, NULL);
    pthread_create(&m_thread, NULL, &audio_thread, NULL);
    main_controller.activate();
    return Controller::init_controller(argc, argv);
}


int exit_midi(){
  pthread_join(piano_midi_thread, NULL);
  pthread_join(m_thread, NULL);
  close(MidiFD);
  for(int i = 0; i < synth_algorithms.size(); i++){
    delete synth_algorithms[i];
  }
  return Controller::exit_controller();
}

int init_alsa(){
    int i;
    int err;
    unsigned int sample_rate = 44100;
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_sw_params_t *sw_params;
    snd_pcm_uframes_t buffer_size = 441;
    
    playback_handle = 0;
    if ((err = snd_pcm_open (&playback_handle, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        fprintf (stderr, "cannot open audio device %s\n", snd_strerror (err));
        return EXIT_FAILURE;
    }
    
    snd_pcm_hw_params_malloc (&hw_params);
    snd_pcm_hw_params_any (playback_handle, hw_params);
    snd_pcm_hw_params_set_access (playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format (playback_handle, hw_params, SND_PCM_FORMAT_FLOAT_LE);
    snd_pcm_hw_params_set_rate_near (playback_handle, hw_params, &sample_rate, NULL);
    snd_pcm_hw_params_set_channels (playback_handle, hw_params, CHANNELS); //this thing
    snd_pcm_hw_params_set_buffer_size_near(playback_handle, hw_params, &buffer_size);
    snd_pcm_hw_params (playback_handle, hw_params);
    snd_pcm_hw_params_free (hw_params);
    
    /* 
       tell ALSA to wake us up whenever 4096 or more frames
       of playback data can be delivered. Also, tell
       ALSA that we'll start the device ourselves.
    */
	
    snd_pcm_sw_params_malloc (&sw_params);
    snd_pcm_sw_params_current (playback_handle, sw_params);
    snd_pcm_sw_params_set_avail_min (playback_handle, sw_params, 441);
    snd_pcm_sw_params_set_start_threshold (playback_handle, sw_params, 0U);
    snd_pcm_sw_params (playback_handle, sw_params);
    snd_pcm_sw_params_free(sw_params);
    
    /* the interface will interrupt the kernel every 4096 frames, and ALSA
       will wake up this program very soon after that.
    */
    
    snd_pcm_uframes_t buf_size;
    snd_pcm_uframes_t period_size;
    snd_pcm_get_params(playback_handle, &buf_size, &period_size);
    //printf("%d derp %d\n", buf_size, period_size);    
    snd_pcm_prepare(playback_handle);
    
    return EXIT_SUCCESS;
}

int exit_alsa(){
    snd_pcm_close (playback_handle);
    snd_config_update_free_global();
    return EXIT_SUCCESS;
}

void *midi_loop(void *ignoreme){
  int sustain = 0;
  int bend = 64;
  MidiByte packet[4];
  std::queue<unsigned char> incoming;
  unsigned char temp;
  while(is_window_open()){
    if(read(MidiFD, &temp, sizeof(temp)) <= 0){
      usleep(10);
      continue;
    }
    else{
      incoming.push(temp);
    }
    
    if(incoming.size() >= 3){
      packet[0] = incoming.front(); incoming.pop();
      packet[1] = incoming.front(); incoming.pop();
      packet[2] = incoming.front(); incoming.pop();
    }
    else continue;
    
    printf("keyboard %d %d %d\n", packet[0], packet[1], packet[2]);
    
    switch(packet[0]){
    case(MIDI_NOTE_OFF):
      if(sustain > 64){
	midiNotesSustained[packet[1]] = midiNotesPressed[packet[1]];      
      }
      midiNotesPressed[packet[1]] = 0;
      break;
    case(MIDI_NOTE_ON):
      midiNotesPressed[packet[1]] = packet[2];            
      break;
    case(PEDAL):
      sustain = packet[2];
      if(sustain < 64){
	memset(midiNotesSustained, 0, sizeof(midiNotesSustained) * sizeof(midiNotesSustained[0]));
      }
      break;
    case(PITCH_BEND):
      bend = packet[2];
      break;
    }
  }
  printf("Piano Thread is DEADBEEF\n");
  return 0;
}

/*    
      for(int t = 0; t < 2*88200; t++ ){
        if(t < 44100) ep = pow(M_E, -t/4410.0);
        mo = ep*(sin(11000*omega*t));
        sound[t] = zero_filter.tick(.1*sin(440*t*omega + 100*mo));
        }*/ //frequency modulation is cool as shit.

/*    for(int t = 0; t < 44100; t++ ){
        count = 4*abs(.5 - (t % (int)(44100 / gain)) * gain/44100.0) - 1;
        if(t < 44100) ep = pow(M_E, -t/4410.0);
        mo = ep*count;
        sound[t] = zero_filter.tick(.5*sin(440*t*omega*mo));
        }*/ //triangle wave is best wave

/*    for(int t = 0; t < 44100; t++ ){
        count = sin(pitch*omega*t);
        if(count > .95) count = 0;
        else if(count < -.95) count = 0;
        
//        if(t > 4410) count = 0;
//        mo = count*(sin(440*omega*t));
        sound[t] = count;
        }*/

/*if(t < 1920){
            clear_wave();
//            printf("%f\n", temp1);
            plot_wave(0, temp1/(gain*2));
            plot_wave(1, temp2/(2*2));
            plot_wave(2, sound[t]);
            usleep(1000);

        }
	}*/
    
//    for(int i = 0; i < 100; i++){
//        set_wave_buffer(0, sound+(i*441));
//    }
