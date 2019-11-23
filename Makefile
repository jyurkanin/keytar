CXX=g++
CFLAGS=-I/home/justin/code/keytar/stk-4.6.0/include/ -D__LINUX_ALSA_ -g -Wall
DEPS = filter.h audio_engine.h wave_window.h synth.h fft.h controller.h operator.h scanner.h
OBJ = filter.o audio_engine.o sy.o wave_window.o synth.o fft.o controller.o operator.o scanner.o stk-4.6.0/src/Stk.o stk-4.6.0/src/ADSR.o stk-4.6.0/src/OneZero.o stk-4.6.0/src/SineWave.o stk-4.6.0/src/BlitSquare.o stk-4.6.0/src/Iir.o 

LIBS=-lm -lpthread -lasound -lX11

%.o: %.cpp $(DEPS)
	$(CXX) -c -o $@ $< $(CFLAGS)


all: $(OBJ)
	$(CXX) -o sy $^ $(CFLAGS) $(LIBS)
test:
	$(CXX) -o test test.c $(CFLAGS) $(LIBS)

