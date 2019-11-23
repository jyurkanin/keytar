#include "scanner.h"
#include <string.h>
#include <math.h>
#include <cstdio>
#include <stdlib.h>

/*
 * Cool.
 */

void broken(){}

//vector norm
float norm(float *vec, int len){
  float sum = 0;
  for(int i = 0; i < len; i++){
    sum += vec[i] * vec[i];
  }
  return sqrtf(sum); 
}

float dist(float *vec1, float *vec2, int len){
  float d[len];
  float sum = 0;
  for(int i = 0; i < len; i++){
    d[i] = vec1[i] - vec2[i];
    sum += d[i]*d[i];
  }
  return sqrt(sum);
}

Scanner::Scanner(int size){
  srand(1);
  update_freq_ = 441; //Hz
  z_table = new float[size+2]; //includes boundary conditions;
  zd_table = new float[size];
  zdd_table = new float[size];

  x_table = new float[size+2]; //pretty much always going to want x_table[0] to be 0
  for(int i = 1; i < size + 2; i++){
    //    x_table[i] = x_table[i-1] + 2*(float)rand()/RAND_MAX;//i*2;
    x_table[i] = i*2;
  }

  //  z_table[0] = 0;
  //  z_table[size+1] = 0;
  hammer_table = new float[size];
  //hammer_table[0] = 1; //basically an impulse
  for(int i = 0; i < size/2; i++){
    //    hammer_table[i] = sin((i+1)*2*M_PI/(size+1))*10;
    hammer_table[i] = 10;
  }
  
  z_center = new float[size];
  z_damping = new float[size];
  x_stiffness = new float[size+1];
  for(int i = 0; i < size+1; i++){
    x_stiffness[i] = 2 * (float)rand()/RAND_MAX;
  }
  masses = new float[size];
  for(int i = 0; i < size; i++){
    masses[i] = 4;//2*(float)rand()/RAND_MAX;;
    z_center[i] = 0.01;
    z_damping[i] = 0; //seems to work well
  }
  
  table_size = size;
}

Scanner::~Scanner(){
  delete[] z_table;
  delete[] zd_table;
  delete[] zdd_table;
  delete[] x_table;
  delete[] hammer_table;
  delete[] z_center;
  delete[] z_damping;
  delete[] x_stiffness;
  delete[] masses;
}

void Scanner::draw_scanner(Display *dpy, Window w, GC gc){
  XSetForeground(dpy, gc, 0);
  XFillRectangle(dpy, w, gc, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

  //this first part draws the simulated nodes.
  //It takes up the top half of the screen.
  int x_pos;
  int x_pos2;
  int y_pos;
  float length = x_table[table_size+1]; //get the farthest point
  for(int i = 0; i < table_size+1; i++){
    x_pos = x_table[i]*(SCREEN_WIDTH-10)/length;
    x_pos2 = x_table[i+1]*(SCREEN_WIDTH-10)/length;
    y_pos = SCREEN_HEIGHT/4;
    XSetForeground(dpy, gc, 0xFF);
    XDrawLine(dpy, w, gc, x_pos, y_pos + (z_table[i]*20), x_pos2, y_pos + (z_table[i+1]*20));
    XSetForeground(dpy, gc, 0xFF00);    
    XDrawPoint(dpy, w, gc, x_pos, y_pos + (z_table[i]*20));
  }
  XFlush(dpy);
}

void Scanner::update_point(int i){
  //Step 1. Calc accelerations from forces.
  //Step 2. Integrate

  if(z_table[i+1] > 0)
    broken();
  //distance from current node to prev node
  float spring_len;
  float displacement;
  float prev_dist[2] = {x_table[i] - x_table[i+1], z_table[i] - z_table[i+1]};         //distance from prev to current node
  spring_len = hypot(prev_dist[0], prev_dist[1]);                                      //get length of spring
  displacement = spring_len - EQ_LEN;
  float Fp_k = (x_stiffness[i]*displacement*displacement) * (prev_dist[1] / spring_len);   //spring force in z direction

  float next_dist[2] = {x_table[i+2] - x_table[i+1], z_table[i+2] - z_table[i+1]};     //distance from next to current node
  spring_len = hypot(next_dist[0], next_dist[1]);                                      //get length of spring
  displacement = spring_len - EQ_LEN;
  float Fn_k = (x_stiffness[i+1]*displacement*displacement) * (next_dist[1] / spring_len); //spring force in z direction
  
  float F_c = 0;//-z_table[i+1]*z_center[i];                                               //damping centering force
  float F_b = -zd_table[i]*z_damping[i];

  zdd_table[i] = (Fp_k + F_c + F_b + Fn_k) / masses[i];
}

float Scanner::tick(float pitch){
  if(t_ > (k_*44100.0/update_freq_)){
    for(int i = 0; i < table_size; i++){
      update_point(i); //this updates accelerations.
    }    
    for(int i = 0; i < table_size; i++){
      zd_table[i] += (zdd_table[i] * TIMESTEP);
      z_table[i+1] +=  (zd_table[i] * TIMESTEP);
    }
    
    k_++; //counts the number of updates
  }
  t_++;

  //linear interpolation
  float index = fmod((pitch*t_*(1+table_size)/44100.0), 1+table_size); //loop speed is based on pitch
  int lower = ((int) index);
  float diff = index - lower;
  float sample = z_table[lower]*(1-diff) + z_table[lower+1]*(diff);

  return sample * .005;
  //  return z_table[lower]*.005; //sample*.05;
}

//apply hammer
void Scanner::strike(){
  //skip boundary condition
  t_ = 0;
  k_ = 0;
  memcpy(&z_table[1], hammer_table, table_size*sizeof(float));
  for(int i = 0; i < table_size+2; i++){
    printf("%f ", z_table[i]);
  }
  printf("\n");
}

void Scanner::reset(){
  t_ = 0;
  k_ = 0;
  
  memset(z_table, 0, table_size*sizeof(float));
  memset(zd_table, 0, table_size*sizeof(float));
  memset(zdd_table, 0, table_size*sizeof(float));
  
}

void Scanner::setFreq(float freq){
  update_freq_ = freq;
}
