#include "fft.h"
#include <complex>
#include <iostream>
#include <string.h>
#include <vector>

const double PI = 3.141592653589793238460;
using cd = std::complex<double>;
using std::vector;

void fft(vector<cd> & a, bool invert) {
    int n = a.size();
    if (n == 1)
        return;

    vector<cd> a0(n / 2), a1(n / 2);
    for (int i = 0; 2 * i < n; i++) {
        a0[i] = a[2*i];
        a1[i] = a[2*i+1];
    }
    fft(a0, invert);
    fft(a1, invert);

    double ang = 2 * PI / n * (invert ? -1 : 1);
    cd w(1), wn(cos(ang), sin(ang));
    for (int i = 0; 2 * i < n; i++) {
        a[i] = a0[i] + w * a1[i];
        a[i + n/2] = a0[i] - w * a1[i];
        if (invert) {
            a[i] /= 2;
            a[i + n/2] /= 2;
        }
        w *= wn;
    }
}

/*
void ifft(CArray& x){
    x = x.apply(std::conj);
    fft( x );
    x = x.apply(std::conj);
    x /= x.size();
}
*/

void calc_fft(float* input, float* mag, float* phase, int len, int zero_padding){
    vector<cd> test;
    for(int i = 0; i < len; i++){
        test.push_back(input[i]);
    }
    for(int i = 0; i < zero_padding; i++){
        test.push_back(0);
    }
    
    fft(test, 0);
    float max = -1;
    for(int i = 0; i < len+zero_padding; i++){    
        mag[i] = abs(test[i]);
        if(mag[i] > max) max = mag[i];
    }
    for(int i = 0; i < len+zero_padding; i++){
        if(mag[i] > max/100) phase[i] = arg(test[i]);
        else phase[i] = 0;
    }
}

void calc_ifft(float* output, float* mag, float* phase, int len, int zero_padding){
    vector<cd> test;
    for(int i = 0; i < len+zero_padding; i++){
        test.push_back(std::polar(mag[i], phase[i]));
    }
    
    fft(test, 1);
    for(int i = 0; i < len; i++){    
        output[i] = real(test[i]) + 1;
    }
}



void calc_fft(std::deque<float>::iterator buffer, float* result, int len, int zero_padding, int window){
    vector<cd> test;
    float num = log2(len);
    num = ceil(num);
    
    for(int i = 0; i < len; i++){
        if(window)
            test.push_back((*buffer)*cos(3.14159*i/(len-1)));
        else
            test.push_back(*buffer);
        
        buffer++;
    }
    for(int i = 0; i < zero_padding; i++){
        test.push_back(0);
    }
    
    fft(test, 0);
    for(int i = 0; i < len+zero_padding; i++){    
        result[i] = norm(test[i]);
    }
}

void apply_hann_window(float* input, int len){
    for(int i = 0; i < len; i++){
        input[i] *= cos(3.14159*i/((float)(len-1)));
    }
}

