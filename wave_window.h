#pragma once

#include <ncurses.h>
#include <stdlib.h>
#include <vector>
#include <unistd.h>
#include <string>
#include <X11/Xlib.h>
#include <math.h>
#include <pthread.h>
#include "synth.h"
#include "fft.h"
#include "audio_engine.h"
#include "controller.h"
#include "scanner.h"
//#include "circle_scanner.h"

#define NUM_SAMPLES 10
#define W_BUF_SIZE 441 * 10
#define W_SAMPLE_LEN 441
#define MAX_PLOT_LENGTH 958
#define MAX_PLOT_WIDTH 538
#define NUM_PLOTS 3
#define NODE_SIZE 40
#define STATUS_MSG_LEN 50
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

#define MAIN_STATE 0
#define SYNTH_STATE 1 //for selecting a specific synth module
#define SCANNER_STATE 2 //Scanned Synthesis. Its cool.
#define REVERB_STATE 3

void init_window();
void set_capture_fft(float *f_series, int len);
void set_wave_buffer(int,int,int,float*);
int  is_window_open();
void *sy_window_thread(void*);
void plot_wave(int, float);
void del_window();

char* get_string(); //read the keyboard for a string
int get_num();

Scanner* new_scanner();

void clear_wave_buffer();
void set_wave_buffer(float freq, int t, int buf_len, float* values);
void draw_synth_selection_window();
void draw_main_params();
void draw_synth_params(SynthAlg* synth, int active_controler_num);
void draw_main_window();
void draw_synth_window(SynthAlg *synth, int num);
void draw_border();
void draw_wave(int plot_num);
void draw_fft();
void graph_fft(float *fft_plot, int len, int color);
void plot_wave(int plot_num, float value);

void clear_left();
void clear_top_left();
void clear_top_right();
void clear_bottom_left();
void clear_bottom_right();
void clear_wave();
