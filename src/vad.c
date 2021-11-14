#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "pav_analysis.h"
#include "vad.h"

const float FRAME_TIME = 10.0F; /* in ms. */
int DIF_dB_SILENCE = 4; /* Diferencia mínima en dB del ruido incio y silencio */
float PAUSES = 50.0;
const float FM = 16000; /* Frecuencia de muestreo en Hz*/

/* 
 * As the output state is only ST_VOICE, ST_SILENCE, or ST_UNDEF,
 * only this labels are needed. You need to add all labels, in case
 * you want to print the internal state in string format
 */

const char *state_str[] = {
  "UNDEF", "S", "V", "INIT", "MS", "MV"
};

const char *state2str(VAD_STATE st) {
  return state_str[st];
}

/* Define a datatype with interesting features */
typedef struct {
  float zcr;
  float p;
  float am;
} Features;

/* 
 * TODO: Delete and use your own features!
 */

Features compute_features(const float *x, int N) {
  /*
   * Input: x[i] : i=0 .... N-1 
   * Ouput: computed features
   */

  Features feat;
  feat.zcr = compute_zcr(x, N, FM); //Funciones reutilizadas de la práctica P1
  feat.p = compute_power(x, N);
  feat.am = compute_am(x, N);
  return feat;
}

/* 
 * TODO: Init the values of vad_data
 */

VAD_DATA * vad_open(float rate) {
  VAD_DATA *vad_data = malloc(sizeof(VAD_DATA));
  vad_data->state = ST_INIT;
  vad_data->sampling_rate = rate;
  vad_data->frame_length = rate * FRAME_TIME * 1e-3;
  vad_data->frame = 0;

  vad_data->k0 = 0;
  vad_data->k1 = 0;
  vad_data->k2 = 0;

  vad_data->state_time = 0;
  vad_data->last_feature = 0;
  return vad_data;
}

VAD_STATE vad_close(VAD_DATA *vad_data) {
  /* 
   * TODO: decide what to do with the last undecided frames
   */

  VAD_STATE state;

  if (vad_data->state == ST_MAYBEVOICE){
    state = ST_VOICE;

  } else if(vad_data->state == ST_MAYBESILENCE){
    state = ST_SILENCE;

  } else{
    state = vad_data->state;

  }

  free(vad_data);
  return state;
}

unsigned int vad_frame_size(VAD_DATA *vad_data) {
  return vad_data->frame_length;

}

/* 
 * TODO: Implement the Voice Activity Detection 
 * using a Finite State Automata
 */

VAD_STATE vad(VAD_DATA *vad_data, float *x) {

  /* 
   * TODO: You can change this, using your own features,
   * program finite state automaton, define conditions, etc.
   */

  Features f = compute_features(x, vad_data->frame_length);
  //vad_data->last_feature = f.p; /* save feature, in case you want to show */

  switch (vad_data->state) {

  case ST_INIT:

    if(vad_data->last_feature < f.p){ /* Cálculo de umbrales */
      vad_data->k0 += f.p;
      vad_data->frame++;

      if(f.p > vad_data->last_feature + DIF_dB_SILENCE){ /* Inicio del estado Silencio */
        vad_data->k0 = vad_data->k0/vad_data->frame; /* Nivel medio de ref. del ruido de fondo en dB */
        vad_data->k1 = f.p - 2;  /* Nivel medio de silencio en dB */
        vad_data->k2 = vad_data->k1 + 5;/* Nivel seguro de voz en dB*/
        vad_data->state = ST_SILENCE;

      }
    
    }
    
    break;

  case ST_SILENCE:
    if (f.p > vad_data->k1){ /* Nivel de pot. sobre la posibilidad de voz */
      vad_data->state = ST_MAYBEVOICE;
    
    }
    break;

  case ST_VOICE:
    if (f.p < vad_data->k2){ /* Nivel de pot. por debajo de la confirmación de voz */
      vad_data->state = ST_MAYBESILENCE;

    }
    break;

  case ST_MAYBESILENCE:
    vad_data->state_time += FRAME_TIME; /* Actualiza el tiempo que estamos en el estado */

    if (f.p < vad_data->k2 && vad_data->state_time > PAUSES){
      vad_data->state = ST_VOICE;
      vad_data->state_time = 0; /* Inicialización */

    } else if(f.p < vad_data->k1){
      vad_data->state = ST_SILENCE;
      vad_data->state_time = 0; /* Inicialización */

    }

  break;

case ST_MAYBEVOICE:
    vad_data->state_time += FRAME_TIME; /* Actualiza el tiempo que estamos en el estado */

    if(f.p > vad_data->k1 && vad_data->state_time > PAUSES){
      vad_data->state = ST_SILENCE;
      vad_data->state_time = 0; /* Inicialización */

    } else if(f.p > vad_data->k2){
      vad_data->state = ST_VOICE;
      vad_data->state_time = 0; /* Inicialización */

    }

  break;

  case ST_UNDEF:
    break;
  }

  vad_data->last_feature = f.p; /* Actualización valor potencia anterior */

  if (vad_data->state == ST_SILENCE || vad_data->state == ST_VOICE){
        return vad_data->state;

  } else{
    return ST_UNDEF;

  }

}

void vad_show_state(const VAD_DATA *vad_data, FILE *out) {
  fprintf(out, "%d\t%f\n", vad_data->state, vad_data->last_feature);
}
