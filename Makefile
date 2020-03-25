CXX=g++
CFLAGS= -D__LINUX_ALSA_ -g -Wall
DEPS = filter.h audio_engine.h wave_window.h synth.h fft.h controller.h operator.h scanner.h reverb.h 
OBJ = filter.o audio_engine.o sy.o wave_window.o synth.o fft.o controller.o operator.o scanner.o reverb.o

LIBS=-lm -lpthread -lasound -lX11

%.o: %.cpp $(DEPS)
	$(CXX) -c -o $@ $< $(CFLAGS)


all: $(OBJ)
	$(CXX) -o sy $^ $(CFLAGS) $(LIBS)
test:
	$(CXX) -o test test.c $(CFLAGS) $(LIBS)
train:
	$(CXX) -o train aencoder.cpp fft.cpp -lm -g -Wall -lX11
