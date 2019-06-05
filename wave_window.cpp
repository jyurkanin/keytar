#include "wave_window.h"
#include <string.h>
#include <X11/Xutil.h>
#include <ctype.h>

Display *dpy;
Window w;
GC gc;
//start indices
int start_index[NUM_PLOTS];
int end_index[NUM_PLOTS];
//a ring buffer
float plots[NUM_PLOTS][MAX_PLOT_LENGTH];
pthread_t w_thread;

int speed = 1;
//circle buffer
int buf_write_index=0;
int buf_read_index=0;
int is_buf_empty=0;
float wave_buffer[W_BUF_SIZE];
int is_window_alive;


void init_window(){
    dpy = XOpenDisplay(0);
    w = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, MAX_PLOT_LENGTH, MAX_PLOT_WIDTH, 0, 0, 0);
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

void set_wave_buffer(int plot, float * values){ //got to increment the write index, advance end of ring buffer
//    printf("%d\t%d\n", buf_write_index, buf_read_index);
    if(is_buf_empty){
        memcpy(wave_buffer, values, sizeof(float)*W_SAMPLE_LEN);
        buf_write_index = 1; //write index is the end of the ring buffer
        buf_read_index = 0; //read index is the start of the ring buffer
        is_buf_empty = 0;
    }
    else if(buf_write_index != buf_read_index){
        memcpy(wave_buffer+(buf_write_index*W_SAMPLE_LEN), values, sizeof(float)*W_SAMPLE_LEN);
        buf_write_index++; //write index is the end of the ring buffer
        buf_write_index = buf_write_index % NUM_SAMPLES;
    }
    else if(buf_write_index == buf_read_index){ //buffer is full
        //do nothing, I think.
    }
}

int is_window_open(){
    return is_window_alive;
}

void get_string(char* number){ //read the keyboard for a string
    XEvent e;
    char buf[2];
    int count = 0;
    do{
        if(XPending(dpy) > 0){
            XNextEvent(dpy, &e);
            if(e.type == KeyPress){
                XLookupString(&e.xkey, buf, 1, NULL, NULL);
		number[count] = buf[0];
		count++;
            }
        }
    } while(buf[0] != '\n');
}

int get_num(){ //read the keyboard for a number
    XEvent e;
    char buf[2];
    char number[128];
    int count = 0;
    int ret;
    do{
        if(XPending(dpy) > 0){
            XNextEvent(dpy, &e);
            if(e.type == KeyPress){
                XLookupString(&e.xkey, buf, 1, NULL, NULL);
                if(isdigit(buf[0])){
		  number[count] = buf[0];
		  count++;
		}
            }
        }
    } while(buf[0] != '\n');
    number[count] = 0;
    sscanf(number, "%d", &ret);
    return ret;
}

//could be fucked
void *sy_window_thread(void * arg){
    int sleep_time = 100000;
    //int start_index, end_index;
    XEvent e;
    char buf[2];
    speed = 1;

    char status_msg[50];
    char cmd_state = MAIN_STATE;
    
    float *temp_wave;
    int length;
    
    int synth_num;
    SynthAlg* synth;
    int alg_num;
    
    while(1){
        if(XPending(dpy) > 0){
            XNextEvent(dpy, &e);
            if(e.type == KeyPress){
                XLookupString(&e.xkey, buf, 1, NULL, NULL);

		switch(cmd_state){
		case MAIN_STATE:
		  draw_main_window();
		  
		  if(isdigit(buf[0])){ //switch oscillator, 0-9
		    sscanf(buf, "%d", &synth_num);
		    synth = getSynth(synth_num);
		    if(synth){
		      synth->controller.activate();
		      cmd_state = SYNTH_STATE;
		    }
		  }
		  else{
		    switch(buf[0]){
		    case '+':
		      speed *= 2;
		      break;
		    case '-':
		      speed /= 2;
		      if(speed == 0) speed = 1;
		      break;
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
		      synth->controller.activate();
		      break;
		    case 'q': //quit
		      del_window();
		      return 0;
		    }
		  }
		  break;
		  
		case SYNTH_STATE:
		  draw_synth_window(synth);
		  switch(buf[0]){
		  case 'x':
		    
		    break;
		  case '+':
		    speed *= 2;
		    break;
		  case '-':
		    speed /= 2;
		    if(speed == 0) speed = 1;
		    break;
		  }
		  break;
		}
            }        
        }
	XFlush(dpy);
    }
}


//this is in the bottom left
int _DRAW_SYNTH_LEN = 4;
int _DRAW_SYNTH_MSG_LEN = 20;
const char * _DRAW_SYNTH_MSG[] =
  {"Sin = 1             ",
   "Sword = 2           ",
   "FM_Simple = 3       ",
   "WaveTable = 4       "};
void draw_synth_selection_window(){
    XSetForeground(dpy, gc, 0);
    
    for(int i = 0; i < _DRAW_SYNTH_LEN; i++){
      XDrawString(dpy, w, gc, 1, 1, _DRAW_SYNTH_MSG[i], _DRAW_SYNTH_MSG_LEN);
    }
}

void draw_main_params(){
  Controller *main = getMainController();
  char line[60];
  memset(line, 0, 60);
  int num_algs = getNumAlgorithms();
  char* name;
  for(int i = 0; i < 9; i++){
    if(i < num_algs){
      name = getSynth(i)->getSynthName();
    }
    else{
      name = "NONE";
    }
    sprintf(line, "[Alg|Knob|Slider] [%s|%d|%d]", name, main->get_knob(i), main->get_slider(i));
    XDrawString(dpy, w, gc, 1, (8*i) + 1, line, 60);
  }
}

void draw_synth_params(SynthAlg* synth){
  char **mapping = new char*[18];
  int len;
  synth->getControlMap(mapping, len);
  for(int i = 0; i < len; i++){
    XDrawString(dpy, w, gc, 1, (8*i) + 2, mapping[i], 50);
  }
  delete mapping;
}

void draw_main_window(){
  draw_border();
  draw_wave(0); //top right
  draw_fft();   //bottom right
  draw_main_params();
  XFlush(dpy);
}

void draw_synth_window(SynthAlg *synth){
  draw_border();
  draw_wave(0); //top right
  draw_fft();   //bottom right
  draw_synth_params(synth);
  XFlush(dpy);
}

void draw_border(){
  XSetForeground(dpy, gc, 0xFF);
  
  XDrawLine(dpy, w, gc, 0, 0, MAX_PLOT_LENGTH, 0);
  XDrawLine(dpy, w, gc, MAX_PLOT_LENGTH, 0, MAX_PLOT_LENGTH, MAX_PLOT_WIDTH);
  XDrawLine(dpy, w, gc, 0, MAX_PLOT_WIDTH, MAX_PLOT_LENGTH, MAX_PLOT_WIDTH);  
  XDrawLine(dpy, w, gc, 0, 0, 0, MAX_PLOT_WIDTH);
  
  XDrawLine(dpy, w, gc, MAX_PLOT_LENGTH/2, MAX_PLOT_WIDTH/2, MAX_PLOT_LENGTH, MAX_PLOT_WIDTH/2);
  XDrawLine(dpy, w, gc, MAX_PLOT_LENGTH/2, 0, MAX_PLOT_LENGTH/2, MAX_PLOT_WIDTH);
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
    XFillRectangle(dpy, w, gc, SCREEN_WIDTH/2 + 1, 0, MAX_PLOT_LENGTH, 128);
}

void draw_wave(int plot_num){
  float avg_value = 0;
  
  int sleep_time = (int) (1000000.0*(speed/44100.0));
  if(is_buf_empty){
    for(int i = 0; i < W_SAMPLE_LEN; i++){
      avg_value += wave_buffer[i];
      if((i % sleep_time) == 0){
	clear_wave();
	plot_wave(plot_num, avg_value / speed);
	avg_value = 0;
      }
    }
    usleep(sleep_time);
  }
  else{
    start_index[plot_num] = W_SAMPLE_LEN*buf_read_index;
    end_index[plot_num] = W_SAMPLE_LEN*(buf_read_index + 1);
    for(int i = start_index[plot_num]; i < end_index[plot_num]; i++){
      avg_value += wave_buffer[i];
      if((i % sleep_time) == 0){
	clear_wave();
	plot_wave(plot_num, avg_value / speed);
	avg_value = 0;
	usleep(sleep_time);
      }
    }
    
    buf_read_index++;
    buf_read_index = buf_read_index % NUM_SAMPLES;
    if(buf_read_index == buf_write_index){
      is_buf_empty = 1;
      memset(wave_buffer, 0, sizeof(float)*W_SAMPLE_LEN);
    }
  }
}

void draw_fft(){
  float *fft_plots = new float[W_SAMPLE_LEN];  
  calc_fft(wave_buffer, fft_plots, W_SAMPLE_LEN);
  int ind;
  int ind_prev;
  
  XSetForeground(dpy, gc, 0xFF);
  for(int i = 1; i < W_SAMPLE_LEN; i++){
    //XFillRectangle(dpy, w, gc, i, (int) 64 + 63*plots[plot_num][ind], 1, 1);
    //TODO: THe number 2 in the next line is a guess
    XDrawLine(dpy, w, gc, i + (SCREEN_WIDTH/2), (int) (SCREEN_HEIGHT*.75) + 2*fft_plots[i], i + (SCREEN_WIDTH/2) - 1, (int) (SCREEN_HEIGHT*.75) + 2*fft_plots[i-1]);
    //XDrawLine(dpy, w, gc, i, (int) 64 + 63*plots[plot_num][ind], i, 64);
  }
  XFlush(dpy);
  delete fft_plots;
}

/*
 * This plots a single point and moves the entire wave over.
 */
void plot_wave(int plot_num, float value){
    unsigned long _color_map[] = {0xFF,0xFF00,0xFF0000};
    plots[plot_num][start_index[plot_num]] = value;
    start_index[plot_num] = (start_index[plot_num]+1) % MAX_PLOT_LENGTH;
    int ind;
    int ind_prev;
    unsigned long color;
    
    int offset_x = SCREEN_WIDTH/2 + 1;
    int offset_y = SCREEN_HEIGHT/4;
    
    XSetForeground(dpy, gc, _color_map[plot_num]);
    ind_prev = start_index[plot_num] % MAX_PLOT_LENGTH;
    for(int i = 1; i < MAX_PLOT_LENGTH-1; i++){
        ind = (start_index[plot_num] + i) % MAX_PLOT_LENGTH;
//        XFillRectangle(dpy, w, gc, i, (int) 64 + 63*plots[plot_num][ind], 1, 1);
        XDrawLine(dpy, w, gc, i + offset_x, (int) offset_y + (SCREEN_HEIGHT/4)*plots[plot_num][ind], i-1 + offset_x, (int) offset_y + (SCREEN_HEIGHT/4)*plots[plot_num][ind_prev]);
        //XDrawLine(dpy, w, gc, i, (int) 64 + 63*plots[plot_num][ind], i, 64);
        ind_prev = ind;
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
