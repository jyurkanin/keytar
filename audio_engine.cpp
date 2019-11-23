#include "audio_engine.h"
#include "fft.h"
#include "filter.h"
#include <fstream>
#include <iostream>
#include <fcntl.h>
#include <vector>
#include <queue>
#include <sys/ioctl.h>

snd_pcm_t *playback_handle;
snd_pcm_t *capture_handle;

pthread_t cap_thread;
pthread_t m_thread;
pthread_t piano_midi_thread;
int MidiFD;
MidiByte midiNotesPressed[0xFF]; /* this records all the notes jus pressed by pitch maximum notes on is KEYS*/
MidiByte midiNotesReleased[0xFF]; //records the notes just released
MidiByte midiNotesSustained[0xFF];
int sustain = 0;

volatile float capture_freq;
float freqs[12] = {27.5, 29.135, 30.868, 32.703, 34.648, 36.708, 38.891, 41.203, 43.654, 46.249, 48.999, 51.913}; // frequencies of the lowest octave
std::vector<SynthAlg*> synth_algorithms;
//SynthAlg *synth_algorithms[MAX_NUM_WAVES];
//int num_algorithms = 0;
Controller main_controller;
Sample sample;
Filter *alg_filters[9];

char cmd_state = MAIN_STATE;
Scanner *scanner;

#define ACOUSTIC_MODE 1
#define MIDI_MODE 0
int input_mode_;



void breakOnMe(){
  //break me on, Break on meeeeeee
}

void set_state(char state){
  cmd_state = state;
}

void set_scanner(Scanner *s){
  scanner = s;
}

void save_program(char* filename){
  char file_path[110];
  sprintf(file_path, "programs/%s", filename);
  std::ofstream out;
  out.open(file_path, std::ofstream::out);
  
  out << synth_algorithms.size() << "\n";
  
  for(unsigned i = 0; i < synth_algorithms.size(); i++){
    out << synth_algorithms[i]->s_func << "\n";
  }

  int slider[9];
  int knob[9];
  int buttons[9];
  
  memcpy(slider, main_controller.slider, sizeof(slider));
  memcpy(knob, main_controller.knob, sizeof(knob));

  for(int j = 0; j < 9; j++){
    out << main_controller.slider[j] << " ";
  }
  for(int j = 0; j < 9; j++){
    out << main_controller.knob[j] << " ";
  }

  int num_controllers;
  
  for(unsigned i = 0; i < synth_algorithms.size(); i++){
    num_controllers = synth_algorithms[i]->getNumControllers();
    out << num_controllers << "\n";
    
    for(int j = 0; j < num_controllers; j++){
      memcpy(slider, synth_algorithms[i]->controllers[j].slider, sizeof(slider));
      memcpy(knob, synth_algorithms[i]->controllers[j].knob, sizeof(knob));
      memcpy(buttons, synth_algorithms[i]->controllers[j].button, sizeof(buttons));
      
      for(int k = 0; k < 9; k++){
	out << slider[k] << " ";
      }
      for(int k = 0; k < 9; k++){
	out << knob[k] << " ";
      }
      for(int k = 0; k < 9; k++){
	out << buttons[k] << " ";
      }
    }
  }

  out.close();
}

void load_program(char* filename){
  for(unsigned i = 0; i < synth_algorithms.size(); i++){
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
  
  int num_controllers;
  for(int i = 0; i < num; i++){
    in >> num_controllers;
    for(int k = 0; k < num_controllers; k++){
      for(int j = 0; j < 9; j++){
	in >> synth_algorithms[i]->controllers[k].slider[j];
      }
      for(int j = 0; j < 9; j++){
	in >> synth_algorithms[i]->controllers[k].knob[j];
      }
      for(int j = 0; j < 9; j++){
	in >> synth_algorithms[i]->controllers[k].button[j];
      }
    }
  }
  
}

int getNumAlgorithms(){
  return synth_algorithms.size();
}

SynthAlg* getSynth(int num){
  if(num < (int) synth_algorithms.size())
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

/* n = which note
 * t = frames since pressed,
 * s = frames since Release state is entered
 * 
 */
float synthesize(int n, int t, int s, int volume, int& state){
  float freq = freqs[(n-21) % 12] * (1 << (1+(int)(n-21)/12));
  return synthesize(freq, t, s, volume, state);
}

float synthesize_portamento(int curr, int last, int t, int s, int volume, int& state){
  static float p_freq = 0; //portamento freq
  float curr_freq = freqs[(curr-21) % 12] * (1 << (1+(int)(curr-21)/12));
  
  if(last == -1){ //no note was being held down. So immedietely play curr_freq.
    p_freq = curr_freq;
  }
  else{
    p_freq += .001*(curr_freq - p_freq);
  }
  
  return scanner->tick(p_freq)*volume;  
}

float synthesize(float freq, int t, int s, int volume, int& state){
  float sample = 0;
  
  /*  for(int i = 0; i < 9; i++){
    temp = main_controller.get_knob(i);
    if(temp != old_knobs[i]){
      old_knobs[i] = temp;
      alg_filters[i]->setCutoff(temp*.0078125*20000);
    }
    }*/

  for(unsigned i = 0; i < synth_algorithms.size(); i++){
    //alg_filters[i]->tick
    sample += main_controller.get_slider(i) * compute_algorithm(freq, t, s, volume, i, state) / 128.0f;
  }
  return sample;
}

float compute_algorithm(int n, int t, int s, int volume, int alg_num, int& state){
  float freq = freqs[(n-21) % 12] * (1 << (1+(int)(n-21)/12));
  return compute_algorithm(freq, t, s, volume, alg_num, state);
}
float compute_algorithm(float freq, int t, int s, int volume, int alg_num, int& state){
  if(alg_num >= (int) synth_algorithms.size()) return 0;
  SynthAlg *synth = synth_algorithms[alg_num];
  //todo: implement some of the yamaha dx7 FM algorithms.
  return synth->tick(freq, t, s, state) * volume / 128.0f; //sexy as hell. Thanks c++
}

void setVoice(int n){
  for(unsigned i = 0; i < synth_algorithms.size(); i++){
    synth_algorithms[i]->setVoice(n);
  }
}

void addSynth(int alg){
  if(synth_algorithms.size() >= MAX_NUM_WAVES) return;
  switch(alg){
  case SynthAlg::OSC_ALG:
    synth_algorithms.push_back(new OscAlg());
    break;
  case SynthAlg::SWORD_ALG:
    synth_algorithms.push_back(new SwordAlg());
    break;
  case SynthAlg::FM_SIMPLE_ALG:
    synth_algorithms.push_back(new FmSimpleAlg());
    break;
  case SynthAlg::FM_THREE_ALG:
    synth_algorithms.push_back(new FmThreeAlg());
    break;
  case SynthAlg::WAVE_TABLE_ALG:
    synth_algorithms.push_back(new WaveTableAlg());
    break;
  }  
}

void delSynth(int alg){
  if(alg >= (int)synth_algorithms.size()) return;
  SynthAlg* temp = synth_algorithms[alg];
  synth_algorithms.erase(synth_algorithms.begin()+alg);
  delete temp;
}

void *capture_thread(void *arg){
  int frames_to_deliver = 1764;
  int16_t capture_frames[frames_to_deliver];
  int buff_len = 1764;
  float f_series[buff_len];
  float temp_array[buff_len];
  int err;
  float freq_div = 44100.0/buff_len;
  
  std::deque<float> fft_q;
  int max_array[10];
  float temp_freq;
  
  /*  snd_pcm_start(capture_handle);
  snd_pcm_drop(capture_handle);
  snd_pcm_prepare(capture_handle);*/

  printf("test %d\n", SND_PCM_STATE_PREPARED);
  
  while(is_window_open()){
    while((err = snd_pcm_readi (capture_handle, capture_frames, frames_to_deliver)) != frames_to_deliver && is_window_open()) {
      snd_pcm_prepare (capture_handle);
      fprintf (stderr, "audio interface capture failed (%s)\n", snd_strerror (err));
    }
    
    for(int i = 0; i < buff_len; i++){
      temp_array[i] = capture_frames[i]*5 / 32768.0;
    }
    
    calc_fft(temp_array, f_series, buff_len);
    
    set_capture_fft(f_series, buff_len);
    fft_q.clear();


    
    for(int j = 0; j < 10; j++){
      max_array[j] = -1;
      for(int i = 20; i < buff_len*.125; i++){
	if(f_series[i] > max_array[j]){
	  max_array[j] = i;
	}
      }
      printf("MAX FREQ %d %f %f\n", max_array[j], max_array[j] * freq_div, f_series[max_array[j]]);
      f_series[max_array[j]] = 0;
    }
    
    
    if(max_array[0] == 20) temp_freq = 0;
    else{
      temp_freq = 0;
      for(int i = 0; i < 10; i++){
	temp_freq += max_array[i]*freq_div;
      }
      temp_freq /= 10.0;
    }

    if(capture_freq == 0){
      capture_freq = temp_freq;
    }
    else{
      capture_freq = (.3*temp_freq) + (.7*capture_freq);
    }
  }
  return NULL;
}

void *audio_thread_s(void *arg){
    float sum_frames[441*CHANNELS];
    int err;
    int frames_to_deliver;
    
    while(is_window_open()){
      snd_pcm_wait(playback_handle, 100);
      frames_to_deliver = snd_pcm_avail_update(playback_handle);
      if ( frames_to_deliver == -EPIPE){
	snd_pcm_prepare(playback_handle);
	printf("Epipe\n");
	continue;
      }
      frames_to_deliver = frames_to_deliver > 441 ? 441 : frames_to_deliver;
      memset(sum_frames, 0, CHANNELS*441*sizeof(float));

      printf("CAptured freq %f\n", capture_freq); 
      for(int j = 0; j < frames_to_deliver; j++){
	if(capture_freq){
	  sum_frames[j] = synthesize(capture_freq, sample.index[0], sample.index_s[0], 100, sample.state[0]);
	  sample.index[0]++;
	}
      }
      
      set_wave_buffer(capture_freq, sample.index[0], frames_to_deliver, sum_frames);
      
      while((err = snd_pcm_writei (playback_handle, sum_frames, frames_to_deliver)) != frames_to_deliver && is_window_open()) {
	snd_pcm_prepare (playback_handle);
	fprintf (stderr, "write to audio interface failed (%s)\n", snd_strerror (err));
      }      
    }
    printf("Audio Thread is DEADBEEF\n");
    return NULL;
}

void *audio_thread(void *arg){
    float sum_frames[441*CHANNELS];
    int err;
    int num_on = 0;
    int frames_to_deliver;
    
    int lowest_note;
    int lowest_index;

    int last_note = -1; //for modes that are monophonic and use portamento or whatever its called
    int curr_note = -1;
    int is_monophonic = 0; //scanned synthesis only at the moment
    
    RFilter rfilter(22000, .5);
    LPFilter lpfilter(22000);
    
    printf("we here bish\n");
    while(is_window_open()){
        snd_pcm_wait(playback_handle, 100);
        frames_to_deliver = snd_pcm_avail_update(playback_handle);
        if ( frames_to_deliver == -EPIPE){
	    snd_pcm_prepare(playback_handle);
            printf("Epipe\n");
            continue;
        }

        frames_to_deliver = frames_to_deliver > 441 ? 441 : frames_to_deliver;
        memset(sum_frames, 0, CHANNELS*441*sizeof(float));

	is_monophonic = cmd_state == SCANNER_STATE;
	lowest_note = 0;
	//	if(synth_algorithms.size() > 0){ //this causes the buffer not to fill leading to massive cpu usage. Also it needs to be able to go through the music loop even if no algorithms are present due to scanned and wavetable synth mode
	for(int k = 21; k <= 108; k++){
	  //this is going to assume that the midi thread handles the sustaining. ANd will only issue a note off if the sustain is not active
	  if(midiNotesPressed[k]){
	    if(is_monophonic){
	      sample.volume[k] = midiNotesPressed[k];
	      midiNotesPressed[k] = 0;
	      num_on = 1;
	      scanner->strike();
	      
	      sample.index[k] = 1;
	      sample.index_s[k] = 0;

	      if(curr_note != -1 && k != curr_note){ //so that when you press the same note twice it doesnt shut it off
		sample.index[curr_note] = 0;
	      }
	      last_note = curr_note;
	      curr_note = k;

	    }
	    else{
	      sample.volume[k] = midiNotesPressed[k];
	      midiNotesPressed[k] = 0;
	      num_on++;
	      
	      sample.index[k] = 1; //will cause state to transition to attack
	      sample.index_s[k] = 0;
	    }
	  }
	  else if(midiNotesReleased[k]){
	    midiNotesReleased[k] = 0; //lets just pray we dont have race conditions.
	    sample.index_s[k] = 1; //this will cause the state to transition to Release
	  }
	  
	  
	  if(sample.index[k]){
	    setVoice(k);

	    if(is_monophonic){
	      for(int j = 0; j < frames_to_deliver; j++){
		sum_frames[j] = synthesize_portamento(curr_note, last_note, sample.index[k], sample.index_s[k], sample.volume[k], sample.state[k]);
		sample.index[k]++;
		if(sample.state[k] == Operator::RELEASE) sample.index_s[k]++;
	      }
	      printf("Note %d\n", k);
	    }
	    else{
	      for(int j = 0; j < frames_to_deliver; j++){
		sum_frames[j] += synthesize(k, sample.index[k], sample.index_s[k], sample.volume[k], sample.state[k]);
		sample.index[k]++;
		if(sample.state[k] == Operator::RELEASE) sample.index_s[k]++;
	      }
	    }
	    
	    
	    if(sample.state[k] == Operator::IDLE){
	      curr_note = -1;
	      last_note = -1; //useful for monophonic mode
	      sample.index[k] = 0;
	      sample.index_s[k] = 0;
	      num_on--;
	    }
	    
	    if(!lowest_note){
	      lowest_note = k;
	      lowest_index = sample.index[k];
	    }
	  }
	}
	
	
	if(main_controller.get_button(1)){
	  rfilter.setCutoff(22000*.0078125*(1+main_controller.get_knob(0)));
	  rfilter.setQFactor(main_controller.get_knob(2)/128.0);
	}
	if(main_controller.get_button(0)){
	  lpfilter.setCutoff(22000*.0078125*(1+main_controller.get_knob(0)));
	  lpfilter.setQFactor((1 + main_controller.get_knob(1))/12.8);
	}
	
	if(main_controller.get_button(0) && main_controller.get_button(1)){
	  for(int j = 0; j < frames_to_deliver; j++){
	    sum_frames[j] = lpfilter.tick(sum_frames[j]) + rfilter.tick(sum_frames[j]);
	  }
	}
	else if(main_controller.get_button(0)){
	  for(int j = 0; j < frames_to_deliver; j++){
	    sum_frames[j] = lpfilter.tick(sum_frames[j]);
	  }
	}
	else if(main_controller.get_button(1)){
	  for(int j = 0; j < frames_to_deliver; j++){
	    sum_frames[j] = rfilter.tick(sum_frames[j]);
	  }
	}
	
	if(num_on){
	  set_wave_buffer(lowest_note, lowest_index, frames_to_deliver, sum_frames);
	  while((err = snd_pcm_writei (playback_handle, sum_frames, frames_to_deliver)) != frames_to_deliver && is_window_open()) {
	    snd_pcm_prepare (playback_handle);
	    fprintf (stderr, "write to audio interface failed (%s)\n", snd_strerror (err));
	  }
	}
	else{
	  clear_wave_buffer();
	  usleep(10);
	}
    }
    
    printf("Audio Thread is DEADBEEF\n");
    return NULL;
}


int init_midi(int argc, char *argv[]){
  char *MIDI_DEVICE = argv[1];
  int flags;

  if(argc == 3){
    input_mode_ = MIDI_MODE;
    MidiFD = open(MIDI_DEVICE, O_RDONLY);
    
    flags = fcntl(MidiFD, F_GETFL, 0);
    if(fcntl(MidiFD, F_SETFL, flags | O_NONBLOCK)){
      printf("A real bad ERror %d\n", errno);
    }
    
    if(MidiFD < 0){
      printf("Error: Could not open device %s\n", MIDI_DEVICE);
      exit(-1);
    }
    
    pthread_create(&piano_midi_thread, NULL, &midi_loop, NULL);
    pthread_create(&m_thread, NULL, &audio_thread, NULL);
  }
  else{
    init_record();
    input_mode_ = ACOUSTIC_MODE;
    pthread_create(&m_thread, NULL, &audio_thread_s, NULL);
  }
  
  main_controller.activate();
  return Controller::init_controller(argc, argv);
}


int exit_midi(){
  if(input_mode_ == MIDI_MODE){
    pthread_join(piano_midi_thread, NULL);
    close(MidiFD);
  }
  else{
    exit_record();
  }
  pthread_join(m_thread, NULL);
  for(unsigned i = 0; i < synth_algorithms.size(); i++){
    delete synth_algorithms[i];
  }
  return Controller::exit_controller();
}

int init_record(){
    int err;
    unsigned int sample_rate = 44100;
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_uframes_t buffer_size = 441;

    printf("HEy\n");
    capture_handle = 0;
    if ((err = snd_pcm_open (&capture_handle, "hw:3,0", SND_PCM_STREAM_CAPTURE, 0)) < 0) {
        fprintf (stderr, "cannot open audio device %s\n", snd_strerror (err));
        return EXIT_FAILURE;
    }
    
    snd_pcm_hw_params_malloc (&hw_params);
    snd_pcm_hw_params_any (capture_handle, hw_params);
    snd_pcm_hw_params_set_access (capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format (capture_handle, hw_params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_rate_near (capture_handle, hw_params, &sample_rate, NULL);
    snd_pcm_hw_params_set_buffer_size_near(capture_handle, hw_params, &buffer_size);
    snd_pcm_hw_params_set_channels (capture_handle, hw_params, CHANNELS);
    snd_pcm_hw_params (capture_handle, hw_params);
    snd_pcm_hw_params_free (hw_params);
    
    snd_pcm_uframes_t buf_size;
    snd_pcm_uframes_t period_size;
    snd_pcm_get_params(capture_handle, &buf_size, &period_size);
    snd_pcm_prepare (capture_handle);

    pthread_create(&cap_thread, NULL, &capture_thread, NULL);
    //capture_thread(NULL);
    return EXIT_SUCCESS;  
}

int exit_record(){
  pthread_join(cap_thread, NULL);
  return snd_pcm_close (capture_handle);
}

int init_alsa(){
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
       tell ALSA to wake us up whenever 441 or more frames
       of playback data can be delivered. Also, tell
       ALSA that we'll start the device ourselves.
    */
	
    snd_pcm_sw_params_malloc (&sw_params);
    snd_pcm_sw_params_current (playback_handle, sw_params);
    snd_pcm_sw_params_set_avail_min (playback_handle, sw_params, 441);
    snd_pcm_sw_params_set_start_threshold (playback_handle, sw_params, 0U);
    snd_pcm_sw_params (playback_handle, sw_params);
    snd_pcm_sw_params_free(sw_params);
    
    /* the interface will interrupt the kernel every 441 frames, and ALSA
       will wake up this program very soon after that.
    */
    
    snd_pcm_uframes_t buf_size;
    snd_pcm_uframes_t period_size;
    snd_pcm_get_params(playback_handle, &buf_size, &period_size);
    //printf("%d derp %d\n", buf_size, period_size);    
    snd_pcm_prepare(playback_handle);

    memset(&sample, 0, sizeof(sample));


    /*TEST
    int frames_to_deliver = 441;
    float sum_frames[441];

    while(1){
      if ((err = snd_pcm_wait (playback_handle, 100)) < 0) {
	fprintf (stderr, "poll failed (%s)\n", strerror (errno));
	break;
      }	           
      
      if ((frames_to_deliver = snd_pcm_avail_update (playback_handle)) < 0) {
	if (frames_to_deliver == -EPIPE) {
	  fprintf (stderr, "an xrun occured\n");
	  break;
	} else {
	  fprintf (stderr, "unknown ALSA avail update return value (%d)\n", 
		   frames_to_deliver);
	  break;
	}
      }
      
      frames_to_deliver = frames_to_deliver > 441 ? 441 : frames_to_deliver;
      
      
      
      for(int i = 0; i < frames_to_deliver; i++){
	sum_frames[i] = sin(220*i);
      }
      if ((err = snd_pcm_writei (playback_handle, sum_frames, frames_to_deliver)) < 0) {
	fprintf (stderr, "write failed (%s)\n", snd_strerror (err));
      }
    }
    snd_pcm_drain(playback_handle);
    //END TEST*/
    
    return EXIT_SUCCESS;
}

int exit_alsa(){
    snd_pcm_close (playback_handle);
    snd_config_update_free_global();
    return EXIT_SUCCESS;
}

void *midi_loop(void *ignoreme){
  int bend = 64;
  MidiByte packet[4];
  std::queue<unsigned char> incoming;
  unsigned char temp;
  unsigned char last_status_byte;
  while(is_window_open()){
    if(read(MidiFD, &temp, sizeof(temp)) <= 0){
      usleep(10);
      continue;
    }
    else{
      if(incoming.size() == 0 && !(temp & 0b10000000)){ //so if the first byte in the sequence is not a status byte, use the last status byte.
	incoming.push(last_status_byte);
      }
      incoming.push(temp);
    }
    
    if(incoming.size() >= 3){
      packet[0] = incoming.front(); incoming.pop();
      packet[1] = incoming.front(); incoming.pop();
      packet[2] = incoming.front(); incoming.pop();
    }
    else continue;
    
    printf("keyboard %d %d %d\n", packet[0], packet[1], packet[2]);
    
    last_status_byte = packet[0];
    
    switch(packet[0] & 0b11110000){
    case(MIDI_NOTE_OFF):
      if(sustain < 64){	
	midiNotesReleased[packet[1]] = 1;
      }
      else{
	midiNotesSustained[packet[1]] = 1;
      }
      break;
    case(MIDI_NOTE_ON):
      midiNotesPressed[packet[1]] = packet[2];            
      break;
    case(PEDAL):
      if(sustain >= 64 && packet[2] < 64){ //if the sustain is released.
	for(int i = 0; i < 128; i++){ //releases all the notes and sets the sustained notes to 0.
	  midiNotesReleased[i] |= midiNotesSustained[i];
	  midiNotesSustained[i] = 0;
	}
      }
      sustain = packet[2];
      break;
    case(PITCH_BEND):
      bend = packet[2];
      break;
    default:
      lseek(MidiFD, 0, SEEK_END);
      break;
    }
    
  }
  printf("Piano Thread is DEADBEEF\n");
  return 0;
}
