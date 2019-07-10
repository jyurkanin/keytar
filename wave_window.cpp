#include "wave_window.h"
#include <string.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <ctype.h>
#include <queue>

Display *dpy;
Window w;
GC gc;
//start indices
int start_index[NUM_PLOTS];
int end_index[NUM_PLOTS];
//a ring buffer
float plots[NUM_PLOTS][MAX_PLOT_LENGTH];
pthread_t w_thread;

//circle buffer
int buf_write_index=0;
int buf_read_index=0;
int is_buf_empty=0;
//float wave_buffer[W_SAMPLE_LEN];
//int wave_buffer_len;
int wave_period;
std::deque<float> wave_buffer;

int is_window_alive;


void init_window(){
    dpy = XOpenDisplay(0);
    w = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 0);

    Atom wm_state   = XInternAtom (dpy, "_NET_WM_STATE", true );
    Atom wm_fullscreen = XInternAtom (dpy, "_NET_WM_STATE_FULLSCREEN", true );
    XChangeProperty(dpy, w, wm_state, XA_ATOM, 32, PropModeReplace, (unsigned char *)&wm_fullscreen, 1);
    
    XSelectInput(dpy, w, StructureNotifyMask | ExposureMask | KeyPressMask);
    XClearWindow(dpy, w);
    XMapWindow(dpy, w);
    gc = XCreateGC(dpy, w, 0, 0);
    is_window_alive = 1;
    is_buf_empty = 1;
    
    XEvent e;
    do{
        XNextEvent(dpy, &e);        
    } while(e.type != MapNotify);

    pthread_create(&w_thread, NULL, &sy_window_thread, NULL);    
}

void set_wave_buffer(int n, int t, int buf_len, float* values){ //values is W_SAMPLE_LEN long
  float freqs[12] = {27.5, 29.135, 30.868, 32.703, 34.648, 36.708, 38.891, 41.203, 43.654, 46.249, 48.999, 51.913}; // frequencies of the lowest octave
  float key =  freqs[(n-21) % 12];
  float freq = key  * (1 << (1+(int)(n-21)/12));
  
  float period = wave_period = (44100.0/freq);
  float fstart = period - fmod(t, period);
  int start = round(fstart);
  float temp = buf_len - fstart;
  int len = round(temp - fmod(temp, period));//((buf_len-start)/period)*period; //only copy multiples of the period length.
  
  static int toggle = 0;
  
  if(toggle){
    if(wave_buffer.size() > (3*MAX_PLOT_LENGTH)){
      toggle = 0; //too big. toggle dont recieve any more until queue is empty
    }
  }
  else{
    if(wave_buffer.size() < 2*MAX_PLOT_LENGTH){ //make sure the window always has something to display.
      toggle = 1;
    }
  }
  
  if(toggle){
    for(int i = 0; i < len; i++){
      wave_buffer.push_back(values[i+start]);
    }
  }
}

int is_window_open(){
    return is_window_alive;
}

void get_string(char* number){ //read the keyboard for a string
    XEvent e;
    char buf[2];
    KeySym ks = 0;
    int count = 0;
    do{
        if(XPending(dpy) > 0){
            XNextEvent(dpy, &e);
            if(e.type == KeyPress){
                XLookupString(&e.xkey, buf, 1, &ks, NULL);
		if(ks == 0xFF0D) return;
		number[count] = buf[0];
		XDrawString(dpy, w, gc, (14+count)*8, MAX_PLOT_WIDTH, buf, 1);
		XFlush(dpy);
		count++;
            }
        }
    } while(1);
}

int get_num(){ //read the keyboard for a number
    XEvent e;
    KeySym ks = 0;
    char buf[2] = {0,0};
    char number[128];
    int count = 0;
    int ret = 0;
    do{
        if(XPending(dpy) > 0){
            XNextEvent(dpy, &e);
            if(e.type == KeyPress){
                XLookupString(&e.xkey, buf, 1, &ks, NULL);
                if(isdigit(buf[0])){
		  number[count] = buf[0];
		  count++;
		}
		printf("Keypress %s\n", buf);
            }
        }
    } while(ks != 0xFF0D);
    number[count] = 0;
    sscanf(number, "%d", &ret);
    return ret;
}

//could be fucked
void *sy_window_thread(void * arg){
    //int start_index, end_index;
    XEvent e;
    char buf[2];


    char filename[100];
    char cmd_state = MAIN_STATE;
    
    float *temp_wave;
    int length;
    
    int synth_num;
    SynthAlg* synth;
    int alg_num;
    int controller_num = 0;
    
    while(1){
      switch(cmd_state){
      case MAIN_STATE:
	draw_main_window();
	break;
      case SYNTH_STATE:
	draw_synth_window(synth, controller_num);
	break;
      }
      
      if(XPending(dpy) > 0){
	XNextEvent(dpy, &e);
	if(e.type == KeyPress){
	  XLookupString(&e.xkey, buf, 1, NULL, NULL);
	  
	  switch(cmd_state){
	  case MAIN_STATE:
	    if(isdigit(buf[0])){ //switch oscillator, 0-9
	      synth_num = 0;
	      sscanf(buf, "%d", &synth_num);
	      synth = getSynth(synth_num);
	      if(synth){
		controller_num = 0;
		synth->controllers[controller_num].activate();
		draw_synth_params(synth, controller_num);
		cmd_state = SYNTH_STATE;
	      }
	    }
	    
	    switch(buf[0]){
	    case 'd': //delete a synthalg
	      alg_num = get_num();
	      delSynth(alg_num);
	      break;
	    case 'a': //add a new synthalg
	      cmd_state = SYNTH_STATE;
	      clear_left();
	      draw_synth_selection_window();
	      XFlush(dpy);
	      
	      alg_num = get_num(); //get number from user
	      addSynth(alg_num);
	      synth = getSynth(getNumAlgorithms()-1);
	      synth->controllers[0].activate();
	      controller_num = 0;
	      draw_synth_params(synth, controller_num);
	      break;
	    case 's': //save
	      clear_left();
	      XSetForeground(dpy, gc, 0xFF);
	      XDrawString(dpy, w, gc, 4, MAX_PLOT_WIDTH, "Save as: ", 9);
	      XFlush(dpy);
	      
	      memset(filename, 0, 100);
	      get_string(filename);
	      save_program(filename);
	      draw_main_params();
	      break;
	    case 'l': //load
	      clear_left();
	      XSetForeground(dpy, gc, 0xFF);
	      XDrawString(dpy, w, gc, 4, MAX_PLOT_WIDTH, "Load file: ", 9);
	      XFlush(dpy);
	      
	      memset(filename, 0, 100);
	      get_string(filename);
	      load_program(filename);
	      draw_main_params();
	      break;
	    case 'q': //quit
	      printf("Window Thread is DEADBEEF\n");
	      del_window();
	      return 0;
	    }
	    break;
	    
	  case SYNTH_STATE:
	    if(isdigit(buf[0])){ //switch controller, 0-9
	      sscanf(buf, "%d", &controller_num);
	      if(controller_num < synth->getNumControllers()){
		synth->controllers[controller_num].activate();
	      }
	    }
	    
	    switch(buf[0]){
	    case 'x':
	      cmd_state = MAIN_STATE;
	      activate_main_controller();
	      draw_main_params();
	      break;
	    case 'p':
	      break;
	    }
	    break;
	  }
	}        
      }
      XFlush(dpy);
      usleep(1000);
    }
    
}


//this is in the bottom left
int _DRAW_SYNTH_LEN = 5;
int _DRAW_SYNTH_MSG_LEN = 20;
const char * _DRAW_SYNTH_MSG[] =
  {"Oscillator = 0             ",
   "Sword = 1           ",
   "FM_Simple = 2       ",
   "FM_Three = 3       ",
   "WaveTable = 4       "};
void draw_synth_selection_window(){
    clear_left();
    XSetForeground(dpy, gc, 0xFF);
    
    for(int i = 0; i < _DRAW_SYNTH_LEN; i++){
      XDrawString(dpy, w, gc, 1, 12 + (i*12), _DRAW_SYNTH_MSG[i], _DRAW_SYNTH_MSG_LEN);
    }
}

void draw_main_params(){
  Controller *main = getMainController();
  char line[60];
  int num_algs = getNumAlgorithms();
  char name[20];

  clear_left();
  XSetForeground(dpy, gc, 0xFF);
  
  for(int i = 0; i < 9; i++){
    if(i < num_algs){
      getSynth(i)->getSynthName(name);
    }
    else{
      memset(name, 0, 20);
      memcpy(name, "NONE", 4);
    }

    memset(line, 0, 60);
    sprintf(line, "[Alg|Volume] [%-10s|%3d]", name, main->get_slider(i));
    XDrawString(dpy, w, gc, 1, (12*i) + 16, line, 38);
  }

  memset(line, 32, 60);
  sprintf(line, "[Low Pass Frequency|%f]", main->get_knob(0) * 20000.0 / 128.0);
  XDrawString(dpy, w, gc, 1, 130, line, 38);
}

void draw_synth_params(SynthAlg* synth, int active_controller_num){
  clear_left();
  char mapping[18][50];
  memset(mapping, 32, 18*50*sizeof(char));
  int len;
  int sum = 0;
  int offset_x;
  int offset_y;
  Envelope e;
  XSetForeground(dpy, gc, 0xFF);

  char line[50];
  memset(line, 32, 50);
  sprintf(line, "Active Synth Operator: %d", active_controller_num);
  XDrawString(dpy, w, gc, 1, 12, line, 50);
  
  sum = 2;
  for(int j = 0; j < synth->getNumControllers(); j++){
    synth->getControlMap(mapping, len, j);
    for(int i = 0; i < len; i++){
      XDrawString(dpy, w, gc, 1, (12*(i+sum)) + 12, mapping[i], 50);
    }
    sum += len+2;
    
    e = synth->getEnvelope(j);
    offset_x = 300;
    offset_y = 12*(sum-2);
    
    XDrawLine(dpy, w, gc, offset_x, offset_y, offset_x + (int)(e.attack_time*64), offset_y - (int)(e.attack_level*64));
    XDrawLine(dpy, w, gc, offset_x + (int)(e.attack_time*64), offset_y - (int)(e.attack_level*64), offset_x + (int)((e.attack_time + e.decay_time)*64), offset_y - (int)(e.sustain_level*64));
    XDrawLine(dpy, w, gc, offset_x + (int)((e.attack_time + e.decay_time)*64), offset_y - (int)(e.sustain_level*64), offset_x + (int)((e.attack_time + e.decay_time)*64) + 64, offset_y - (int)(e.sustain_level*64));
    XDrawLine(dpy, w, gc, offset_x + (int)((e.decay_time + e.attack_time)*64) + 64, offset_y - (int)(e.sustain_level*64), offset_x + (int)((e.decay_time + e.attack_time + e.release_time)*64)+ 64, offset_y); 
  }
}

void draw_main_window(){
  draw_border();
  draw_fft();   //bottom right
  draw_wave(0); //top right
  if(Controller::has_new_data()){
    draw_main_params();
  }
  XFlush(dpy);
}

void draw_synth_window(SynthAlg *synth, int num){
  draw_border();
  draw_fft();   //bottom right
  draw_wave(0); //top right
  if(Controller::has_new_data()){
    draw_synth_params(synth, num);
  }
  XFlush(dpy);
}

void draw_border(){
  XSetForeground(dpy, gc, 0xFF);
  
  XDrawLine(dpy, w, gc, 0, 0, SCREEN_WIDTH-1, 0);
  XDrawLine(dpy, w, gc, SCREEN_WIDTH-1, 0, SCREEN_WIDTH-1, SCREEN_HEIGHT-1);
  XDrawLine(dpy, w, gc, 0, SCREEN_HEIGHT-1, SCREEN_WIDTH-1, SCREEN_HEIGHT-1);
  XDrawLine(dpy, w, gc, 0, 0, 0, SCREEN_HEIGHT-1);
  
  XDrawLine(dpy, w, gc, SCREEN_WIDTH/2, SCREEN_HEIGHT/2, SCREEN_WIDTH-1, SCREEN_HEIGHT/2);
  XDrawLine(dpy, w, gc, SCREEN_WIDTH/2, 0, SCREEN_WIDTH/2, SCREEN_HEIGHT-1);
}

void clear_left(){
  XSetForeground(dpy, gc, 0);
  XFillRectangle(dpy, w, gc, 1, 1, MAX_PLOT_LENGTH, MAX_PLOT_WIDTH*2 +1);
}

void clear_top_left(){
  XSetForeground(dpy, gc, 0);
  XFillRectangle(dpy, w, gc, 1, 1, MAX_PLOT_LENGTH, MAX_PLOT_WIDTH);
}
void clear_top_right(){
  XSetForeground(dpy, gc, 0);
  XFillRectangle(dpy, w, gc, (SCREEN_WIDTH/2)+1, 1, MAX_PLOT_LENGTH, MAX_PLOT_WIDTH);
}

void clear_bottom_left(){
  XSetForeground(dpy, gc, 0);
  XFillRectangle(dpy, w, gc, 1, (SCREEN_HEIGHT/2)+1, MAX_PLOT_LENGTH, MAX_PLOT_WIDTH);
}

void clear_bottom_right(){
  XSetForeground(dpy, gc, 0);
  XFillRectangle(dpy, w, gc, (SCREEN_WIDTH/2)+1, (SCREEN_HEIGHT/2)+1, MAX_PLOT_LENGTH, MAX_PLOT_WIDTH);
}

//clears top right
void clear_wave(){
    XSetForeground(dpy, gc, 0);
    XFillRectangle(dpy, w, gc, MAX_PLOT_LENGTH + 3, 1, MAX_PLOT_LENGTH, MAX_PLOT_WIDTH);
}

void draw_wave(int plot){
  clear_wave();
  plot_wave(plot, 0);//avg_value / speed);
}

void draw_fft(){
  if(wave_buffer.size() < 2*W_SAMPLE_LEN) return;

  float fft_plot[2*W_SAMPLE_LEN];
  int temp = ((MAX_PLOT_LENGTH - (2*W_SAMPLE_LEN)) / 2) + (SCREEN_WIDTH/2);
  int len = (2*W_SAMPLE_LEN) - fmod(2*W_SAMPLE_LEN, wave_period);
  
  calc_fft(wave_buffer.begin(), fft_plot, len);
  
  clear_bottom_right();
  XSetForeground(dpy, gc, 0xFF);
  
  int x_pos;
  for(int i = 0; i < len/2; i++){    
    x_pos = temp + (60*log(i)) + i;
    XDrawLine(dpy, w, gc, x_pos, SCREEN_HEIGHT - (fft_plot[i]*.1), x_pos, SCREEN_HEIGHT);
  }
}

void plot_wave(int plot_num, float value){
    unsigned long _color_map[] = {0xFF,0xFF00,0xFF0000};
    int offset_x = SCREEN_WIDTH/2 + 1;
    int offset_y = SCREEN_HEIGHT/4;
    int temp = MAX_PLOT_LENGTH < wave_buffer.size() ? MAX_PLOT_LENGTH : wave_buffer.size(); //get the limiting factor
    int display_len = wave_period == 0 ? 0 : temp - fmod(temp, wave_period); //make it a multiple of the period.
    
    usleep(100000);
    XSetForeground(dpy, gc, _color_map[plot_num]);
    
    float point;
    float prev_point = 0;
    for(int i = 1; i < display_len; i++){
      point = wave_buffer.front();
      wave_buffer.pop_front();
      //        XFillRectangle(dpy, w, gc, i, (int) 64 + 63*plots[plot_num][ind], 1, 1);
      XDrawLine(dpy, w, gc, i + offset_x, (int) offset_y + (SCREEN_HEIGHT*.1)*point, i-1 + offset_x, (int) offset_y + (SCREEN_HEIGHT*.1)*prev_point);
      //XDrawLine(dpy, w, gc, i, (int) 64 + 63*plots[plot_num][ind], i, 64);
      prev_point = point;
    }
    XFlush(dpy);
}

void del_window(){
    XDestroyWindow(dpy, w);
    XCloseDisplay(dpy);
    is_window_alive = 0;
    return;
}




/*void t_window(){
    float omega = 2*M_PI/44100.0;
    float ep, mo;
    for(int t = 0; t < 2*88200; t++ ){
        if(t < 44100) ep = pow(M_E, -t/4410.0);
        mo = ep*(sin(11*omega*t));
        plot_wave(0, sin(440*t*omega + 100*mo));
        usleep(10000);
    } //frequency modulation is cool as shit.
    }*/
