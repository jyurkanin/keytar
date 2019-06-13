#pragma once

#include <stdio.h>
#include <fcntl.h>
#include <linux/soundcard.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "stk-4.6.0/include/OneZero.h"
#include "stk-4.6.0/include/BlitSquare.h"
#include "stk-4.6.0/include/Generator.h"
#include "stk-4.6.0/include/SineWave.h"
#include "stk-4.6.0/include/Iir.h"
#include "stk-4.6.0/include/Stk.h"
#include <alsa/asoundlib.h>
#include <math.h>
#include <stdlib.h>
#include <complex.h>
#include <sys/time.h>
#include "wave_window.h"
#include "ADSR_S.h"
#include "synth.h"
#include "controller.h"


#define MIDI_NOTE_ON 0x90
#define MIDI_NOTE_OFF 0x80
#define PEDAL 176
#define PITCH_BEND 224
#define KEYS 88
#define LOWEST_KEY 21
#define SAMPLE_LEN 4410
#define CHANNELS 1

#define MAIN_CONTROLLER_NUM -1

typedef unsigned char MidiByte; //ugh

typedef struct{
    int index[128];
    //    int period[128];
    int volume[128];
} Sample;

float compute_algorithm(int n, int t, int volume, int alg_num);
float compute_algorithm(float freq, int t, int volume, int alg_num);
float synthesize(int n, int t, int volume);

void activate_main_controller();
void addSynth(int alg);
void delSynth(int num);

void load_program(char* filename);
void save_program(char* filename);

SynthAlg* getSynth(int num);
int getNumAlgorithms();
Controller* getMainController();

int init_alsa();
int exit_alsa();
int init_midi(int argc, char *argv[]);
int exit_midi();
void *midi_loop(void *arg);
void *audio_thread(void *arg);
