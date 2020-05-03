#pragma once
#include <deque>

void calc_fft(std::deque<float>::iterator buffer, float* result, int len, int zero_padding, int window);
void calc_fft(float* input, float* mag, float* phase, int len, int zero_padding);
void calc_ifft(float* output, float* mag, float* phase, int len, int zero_padding);
