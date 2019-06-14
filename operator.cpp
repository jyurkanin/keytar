#define "operator.h"

float tri(float t){
  float temp = fmod(t, PI_2);
  if(temp < HALF_PI){
    return temp / HALF_PI;
  }
  else if(temp < THREE_HALF_PI){
    return (PI - temp) / HALF_PI;
  }
  else if(temp < PI_2){
    return (temp - PI_2) / HALF_PI;
  }
  return -2; //this will never happen chill out.
}

float saw(float t){
  return (fmod(t, PI_2) - PI) / PI;
}

float square(float t){
  return ((int) (fmod(t, PI_2)/PI))*2 - 1; //ranges from 0-1
}

void Operator::tick(float freq, int t){
  switch(waveform/32){
  case 0:
    output = sin(OMEGA*t*freq + (beta_1*beta_2*fm_mod*.0078125));
    break;
  case 1:
    output = tri(OMEGA*t*freq + (beta_1*beta_2*fm_mod*.0078125));
    break;
  case 2:
    output = saw(OMEGA*t*freq + (beta_1*beta_2*fm_mod*.0078125));
    break;
  case 3:
    output = square(OMEGA*t*freq + (beta_1*beta_2*fm_mod*.0078125));
    break;
  }
}

//t is the total time on.
//s is the time since it was released
int Operator::envelope(int t, int s){
  float temp;
  float attack_time_norm = attack_time * .0078125 * 44100.0; //the actual attack time = (attack_time / 128) * 441000
  float decay_time_norm = decay_time * .0078125 * 44100.0;
  float release_time_norm = release_time * .0078125 * 44100.0;

  float attack_level_norm = attack_level*.0078125;
  float sustain_level_norm = sustain_level*.0078125;
  
  //this is going to assume that the note is on.
  if(t < attack_time_norm){
    output *= (t/attack_time_norm)*attack_level_norm;
    return ATTACK;
  }
  else if(t < (attack_time_norm + decay_time_norm)){
    output *= attack_level_norm - (((t - attack_time_norm)/decay_time_norm)*(attack_level_norm - sustain_level_norm));
    return DECAY;
  }
  else if(s == 0){
    output *= sustain_level_norm;
    return SUSTAIN;
  }
  else if(s < release_time_norm){
    output *= sustain_level_norm * (1 - (s / release_time_norm));
    return RELEASE;
  }
  else{
    output = 0;
    return IDLE;
  }
  output;
}
