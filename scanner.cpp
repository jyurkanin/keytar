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

int rainbow(int c){
  static int red = 0x00;
  static int green = 0x00;
  static int blue = 0xFF;
  static int state = 0;

  static int counter = 0;
  counter++;
  if(counter < c){
      int out = 0;
      out = (red << 16) | (blue << 8) | green;
      return out;
  }
  
  counter = 0;

  switch(state){
  case 0:
      green++;
      if(green == 0x8F) state = 1;
      break;
//  case 1:
//      red--;
//      if(red == 0x00) state = 2;
//      break;
  case 1:
      blue--;
      if(blue == 0x00) state = 2;
      break;
  case 2:
      blue++;
      if(blue == 0x8F) state = 3;
      break;
//  case 4:
//      red++;
//      if(red == 0xFF) state = 5;
//      break;
  case 3:
      green--;
      if(green == 0x00) state = 0;
      break;
  }

  int out = 0;
  out = (red << 16) | (green << 8) | blue;
  return out;
  
}

Scanner::Scanner(int size){
  srand(1);
  update_freq_ = 441; //Hz
  z_table = new float[size+2]; //includes boundary conditions;
  zd_table = new float[size];
  zdd_table = new float[size];


  z_table[size+1] = 0;
  x_table = new float[size+2]; //pretty much always going to want x_table[0] to be 0
  for(int i = 1; i < size + 2; i++){
    x_table[i] = i*32;
  }
  xd_table = new float[size];
  xdd_table = new float[size];

  hammer_table = new float[size];
  for(int i = 0; i < size; i++){
//      hammer_table[i] = 0;
//      temp = sinf(M_PI*(i+1)/(size+2));
      hammer_table[i] = 5*sinf(M_PI*(i+1)/(size+2)); //5*fabs(((size+2) / 2.0) - (i+1)) / ((size+2)/2);
  }
//  hammer_table[0] = 10;
  
  z_center = new float[size];
  z_damping = new float[size];
  x_stiffness = new float[size+1];
  for(int i = 0; i < size+1; i++){
    x_stiffness[i] = 4;
  }
  
  masses = new float[size];
  for(int i = 0; i < size; i++){
    masses[i] = .2;//2*(float)rand()/RAND_MAX;;
    z_center[i] = 0.01;
    z_damping[i] = .01; //seems to work well
  }
  
  table_size = size;
}

Scanner::~Scanner(){
  delete[] z_table;
  delete[] zd_table;
  delete[] zdd_table;
  delete[] x_table;
  delete[] xd_table;
  delete[] xdd_table;
  delete[] hammer_table;
  delete[] z_center;
  delete[] z_damping;
  delete[] x_stiffness;
  delete[] masses;
}

float Scanner::getDamping(){
  return z_damping[0];
}

float Scanner::getMass(){
  return masses[0];
}

float Scanner::getTension(){
  return x_table[1] - x_table[0];
}

float Scanner::getStiffness(){
  return x_stiffness[0];
}

void Scanner::draw_scanner(Display *dpy, Window w, GC gc){
//  XSetForeground(dpy, gc, 0);
//  XFillRectangle(dpy, w, gc, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

  //this first part draws the simulated nodes.
  //It takes up the top half of the screen.
//  static int counter = 0x00;
  int x_pos;
  int x_pos2;
  int y_pos;
  float length = x_table[table_size+1]; //get the farthest point
  for(int i = 0; i < table_size+1; i++){
    x_pos = x_table[i]*(SCREEN_WIDTH-10)/length;
    x_pos2 = x_table[i+1]*(SCREEN_WIDTH-10)/length;
    y_pos = SCREEN_HEIGHT/4;
    XSetForeground(dpy, gc, rainbow(512));
    XDrawLine(dpy, w, gc, x_pos, y_pos + (z_table[i]*20), x_pos2, y_pos + (z_table[i+1]*20));
    XSetForeground(dpy, gc, 0xFF0000);    
    XDrawPoint(dpy, w, gc, x_pos, y_pos + (z_table[i]*20));
  }

  XSetForeground(dpy, gc, 0);
  XFillRectangle(dpy, w, gc, 1, SCREEN_HEIGHT/2, 120, 64);

  XSetForeground(dpy, gc, 0xFF0000);
  
  char line[60];
  memset(line, 0, sizeof(line));
  sprintf(line, "Damping: %f", getDamping());
  XDrawString(dpy, w, gc, 1, SCREEN_HEIGHT/2 + 16, line, 30);

  memset(line, 0, sizeof(line));
  sprintf(line, "Mass: %f", getMass());
  XDrawString(dpy, w, gc, 1, SCREEN_HEIGHT/2 + 28, line, 30);

  memset(line, 0, sizeof(line));
  sprintf(line, "Tension: %f", getTension());
  XDrawString(dpy, w, gc, 1, SCREEN_HEIGHT/2 + 40, line, 30);

  memset(line, 0, sizeof(line));
  sprintf(line, "Stiffness: %f", getStiffness());
  XDrawString(dpy, w, gc, 1, SCREEN_HEIGHT/2 + 52, line, 30);
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
  float Fp_kz = (x_stiffness[i]*displacement*displacement) * (prev_dist[1] / spring_len);   //spring force in z direction
  float Fp_kx = (x_stiffness[i]*displacement*displacement) * (prev_dist[0] / spring_len); 

  float next_dist[2] = {x_table[i+2] - x_table[i+1], z_table[i+2] - z_table[i+1]};     //distance from next to current node
  spring_len = hypot(next_dist[0], next_dist[1]);                                      //get length of spring
  displacement = spring_len - EQ_LEN;
  float Fn_kz = (x_stiffness[i+1]*displacement*displacement) * (next_dist[1] / spring_len); //spring force in z direction
  float Fn_kx = (x_stiffness[i+1]*displacement*displacement) * (next_dist[0] / spring_len); 
  
  float F_bz = -zd_table[i]*z_damping[i];
  float F_bx = -xd_table[i]*z_damping[i];

  zdd_table[i] = (Fp_kz + F_bz + Fn_kz) / masses[i];
  xdd_table[i] = (Fp_kx + F_bx + Fn_kx) / masses[i];
}

float Scanner::tick(float pitch){
  if(t_ > (k_*44100.0/update_freq_)){
      for(int i = 0; i < table_size; i++){
          update_point(i); //this updates accelerations.
      }    
      for(int i = 0; i < table_size; i++){
          zd_table[i] += (zdd_table[i] * TIMESTEP);
          z_table[i+1] += (zd_table[i] * TIMESTEP);
          
          xd_table[i] += (xdd_table[i] * TIMESTEP);
          x_table[i+1] += (xd_table[i] * TIMESTEP);
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
  
  memset(xd_table, 0, table_size*sizeof(float));
  memset(xdd_table, 0, table_size*sizeof(float));
  memset(zd_table, 0, table_size*sizeof(float));
  memset(zdd_table, 0, table_size*sizeof(float));
  memcpy(&z_table[1], hammer_table, table_size*sizeof(float));
//  for(int i = 0; i < table_size+2; i++){
//    printf("%f ", z_table[i]);
//  }
//  printf("\n");
}

void Scanner::reset(){
  t_ = 0;
  k_ = 0;
  
  memset(z_table, 0, table_size*sizeof(float));
  memset(zd_table, 0, table_size*sizeof(float));
  memset(zdd_table, 0, table_size*sizeof(float));
  
}

void Scanner::activate(){
  controller.activate();
}

void Scanner::setFreq(float freq){
  update_freq_ = freq;
}

void Scanner::setDamping(float f){
  for(int i = 0; i < table_size; i++){
    z_damping[i] = f;
  }
}

void Scanner::randomize_hammer(){
  for(int i = 0; i < table_size; i++){
    hammer_table[i] = 10 - 20*(float)rand()/RAND_MAX;//sin((i+1)*2*M_PI/(size+1))*5 + 5*i/size;
  }  
}

void Scanner::setMass(float f){
  for(int i = 0; i < table_size; i++){
    masses[i] = f;
  }
}

void Scanner::setTension(float f){
  x_table[0] = 0;
  for(int i = 1; i < table_size+2; i++){
    x_table[i] = x_table[i-1] + f;
  }
}

void Scanner::setStiffness(float f){
  for(int i = 0; i < table_size+1; i++){
    x_stiffness[i] = f;
  }
}
