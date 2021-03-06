#include "scanner.h"
#include "wavfile.h"
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


  z_table[0] = 0;
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
  
  i_vel = 0;
  volume = 0;
  table_size = size;
  hammer_num = 0;
  rainbow_rate = 127;
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

void Scanner::updateParams(){
        setDamping(controller.get_slider(0)/16.0);
        setMass((.1+controller.get_slider(1)/32.0));
        setTension((1+controller.get_slider(2)/16.0));
        setStiffness(controller.get_slider(3)/16.0);
        setVolume(controller.get_slider(4));

        if(controller.get_big_knob() != hammer_num){
            setHammer(controller.get_big_knob());
        }
        
        rainbow_rate = controller.get_big_slider();
        
        i_vel = controller.get_slider(5);
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

float Scanner::getVolume(){
  return volume;
}

float Scanner::getVelocity(){
  return i_vel;
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
    XSetForeground(dpy, gc, rainbow(4*rainbow_rate));
    XDrawLine(dpy, w, gc, x_pos, y_pos + (z_table[i]*20), x_pos2, y_pos + (z_table[i+1]*20));
    XSetForeground(dpy, gc, 0xFF0000);    
    XDrawPoint(dpy, w, gc, x_pos, y_pos + (z_table[i]*20));
  }

  XSetForeground(dpy, gc, 0);
  XFillRectangle(dpy, w, gc, 1, SCREEN_HEIGHT/2, 120, 100);

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

  memset(line, 0, sizeof(line));
  sprintf(line, "Volume: %f", getVolume());
  XDrawString(dpy, w, gc, 1, SCREEN_HEIGHT/2 + 64, line, 30);

  memset(line, 0, sizeof(line));
  sprintf(line, "Initial Velocity: %f Not Used", getVelocity());
  XDrawString(dpy, w, gc, 1, SCREEN_HEIGHT/2 + 76, line, 30);
  
  memset(line, 0, sizeof(line));
  sprintf(line, "Hammer waveform: %d", hammer_num);
  XDrawString(dpy, w, gc, 1, SCREEN_HEIGHT/2 + 88, line, 30);
  
  memset(line, 0, sizeof(line));
  sprintf(line, "Rainbow rate: %d", rainbow_rate);
  XDrawString(dpy, w, gc, 1, SCREEN_HEIGHT/2 + 100, line, 30);
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

  return sample * .005 * volume / 127.0;
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

void Scanner::setVolume(float vol){
  volume = vol;
}

void Scanner::setDamping(float f){
  for(int i = 0; i < table_size; i++){
    z_damping[i] = f;
  }
}

void Scanner::setHammer(int num){
    char fn[100];
    memset(fn, 0, sizeof(fn));
    sprintf(fn, "AKWF/AKWF_%04d.wav", num);

    printf("hammertime\n");
    hammer_num = num;
    if(hammer_num == 0){ //special case.
        for(int i = 0; i < table_size; i++){
            hammer_table[i] = 5*sinf(M_PI*(i+1)/(table_size+2));
        }
    }
    else if(hammer_num == 101){ //another special case I felt was worth including.
        for(int i = 0; i < table_size; i++){
            hammer_table[i] = 5*fabs(((table_size+2) / 2.0) - (i+1)) / ((table_size+2)/2);
        }
    }
    else{ //load the file from AKWF.
        WavFile wavfile(fn); //opens the wav file associated with the waveform given in the string.
        //now you need to linearly interpolate to make the wavfile fit into the hammer table.
        if(table_size == wavfile.data_len){
            for(int i = 0; i < table_size; i++){
                hammer_table[i] = wavfile.data[i]*5;
            }
        }
        else if(table_size < wavfile.data_len){
            for(int i = 0; i < table_size; i++){
                hammer_table[i] = 5*wavfile.data[(int)(i*wavfile.data_len/(float)table_size)];
            }
        }
        else if(table_size > wavfile.data_len){ //need to linearly interpolate.
            float inter;
            float index;
            int l_index; //lower
            int u_index; //upper
            for(int i = 0; i < table_size; i++){
                index = (i*wavfile.data_len/(float)table_size);
                l_index = (int) index;
                u_index = l_index+1;
                if(u_index < wavfile.data_len)
                    hammer_table[i] = 5*((index-l_index)*wavfile.data[u_index] + (u_index-index)*wavfile.data[l_index]); //linear interpolation
                else //edge case
                    hammer_table[i] = 5*((index-l_index)*wavfile.data[wavfile.data_len] + (u_index-index)*wavfile.data[l_index]); 
            }            
        }
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
