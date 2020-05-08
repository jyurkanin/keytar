#include <fann.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include "fft.h"
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <iostream>
#include <math.h>

typedef struct{
    char          RIFF[4];        // RIFF Header      Magic header
    uint32_t      chunkSize;      // RIFF Chunk Size  
    char          WAVE[4];        // WAVE Header      
    char          fmt[4];         // FMT header       
    uint32_t      subchunk1Size;  // Size of the fmt chunk                                
    uint16_t      audioFormat;    // Audio format 1=PCM,6=mulaw,7=alaw, 257=IBM Mu-Law, 258=IBM A-Law, 259=ADPCM 
    uint16_t      numOfChan;      // Number of channels 1=Mono 2=Sterio                   
    uint32_t      samplesPerSec;  // Sampling Frequency in Hz                             
    uint32_t      bytesPerSec;    // bytes per second 
    uint16_t      blockAlign;     // 2=16-bit mono, 4=16-bit stereo 
    uint16_t      bitsPerSample;  // Number of bits per sample      
    char          subchunk2ID[4]; // "data"  string   
    uint32_t      subchunk2Size;  // Sampled data length    
} WavHeader;


class WavFile{
public:
    WavFile(char * filename);
    ~WavFile();
    void print();
    float *data;
    WavHeader header;
};


#define PARSE_VAR(X) if(read(fd, &X, sizeof(X)) != sizeof(X)) printf("error1\n")
WavFile::WavFile(char * filename){
    int fd = open(filename, O_RDONLY);
    if(read(fd, &header, sizeof(header)) != sizeof(header)) printf("error2\n");
    print();
    int16_t *pcm = new int16_t[header.subchunk2Size];
    data = new float[header.subchunk2Size];
    
    if(read(fd, pcm, header.subchunk2Size) != header.subchunk2Size) printf("error3\n");
    for(unsigned i = 0; i < header.subchunk2Size; i++){
        data[i] = (float) pcm[i] / (1 << (header.bitsPerSample - 1));
    }
    delete[] pcm;
}

WavFile::~WavFile(){
    delete[] data;
}

void WavFile::print(){
    printf("RIFF %s\n", header.RIFF);
    printf("chunkSize %d\n", header.chunkSize);
    printf("WAVE %s\n", header.WAVE);
    printf("fmt %s\n", header.fmt);
    printf("subchunk1Size %d\n", header.subchunk1Size);
    printf("audioFormat %d\n", header.audioFormat);
    printf("num_channels %d\n", header.numOfChan);
    printf("samplesPerSec %d\n", header.samplesPerSec);
    printf("bytes per sec %d\n", header.bytesPerSec);
    printf("blockAlign %d\n", header.blockAlign);
    printf("bits per sample %d\n", header.bitsPerSample);
    printf("num samples %d\n", header.subchunk2Size);
}

Display *dpy;
Window w;
GC gc;

void init_plot(){
    dpy = XOpenDisplay(":0.0");
    w = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, 1200, 640, 0, 0, 0);
    
    XSelectInput(dpy, w, StructureNotifyMask | ExposureMask | KeyPressMask);
    XClearWindow(dpy, w);
    XMapWindow(dpy, w);
    gc = XCreateGC(dpy, w, 0, 0);
    
    XEvent e;
    do{
        XNextEvent(dpy, &e);        
    } while(e.type != MapNotify);
}

void plot_wave(float* values, int len){
    XClearWindow(dpy, w);
    float scale = .001;
    printf("len %d\n", len);

    XSetForeground(dpy, gc, 0xFF);
    for(int i = 0; i < len-1; i++){
//        XDrawLine(dpy, w, gc, (i), 320 - log(1+abs(values[i]))*scale, ((i+1)), 320 - log(1+abs(values[i+1]))*scale);
        XDrawLine(dpy, w, gc, (i), 320 - abs(values[i])*scale, (i+1), 320 - abs(values[i+1])*scale);
    }
    XFlush(dpy);
}

int main(){
    int num_puts = 152;
    char fn[100];

    init_plot();

    float *fft_list[100];
    for(int i = 0; i < 100; i++){
        memset(fn, 0, sizeof(fn));
        sprintf(fn, "AKWF/AKWF_%04d.wav", i+1);
        
        WavFile *temp = new WavFile(fn);
        float *fft_coefficients = new float[temp->header.subchunk2Size];
        printf("size %d\n", temp->header.subchunk2Size);
        for(unsigned j = 0; j < temp->header.subchunk2Size; j++){
            temp->data[j] = sinf((157 + 10*i)*3.14159*j/((float)temp->header.subchunk2Size)) + sinf(247*3.14159*j/((float)temp->header.subchunk2Size));
        }
        
        //apply_hann_window(temp->data, temp->header.subchunk2Size);
        calc_fft(temp->data, fft_coefficients, temp->header.subchunk2Size);
        fft_list[i] = fft_coefficients;
        plot_wave(fft_list[i], temp->header.subchunk2Size);
        
//        std::cout << "Press Enter";
        delete temp;
        
    }
    //
}
