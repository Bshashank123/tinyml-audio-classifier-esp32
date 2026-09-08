#ifndef AUDIO_PROCESSOR_H
#define AUDIO_PROCESSOR_H

#include <stdint.h>

#define SAMPLE_RATE 16000
#define AUDIO_DURATION_SEC 1
#define MAX_AUDIO_SAMPLES (SAMPLE_RATE * AUDIO_DURATION_SEC)  // 16000
#define N_FFT 512
#define HOP_LENGTH 500
#define N_MELS 32
#define N_FRAMES 32

// Initialize audio processor (e.g., precompute mel filterbank)
void setupAudioProcessor();

// Compute mel spectrogram
// audio: input 16kHz PCM audio
// numSamples: number of samples (e.g., 16000 for 1 sec)
// output: quantize output [32 x 64] size
void computeMelSpectrogram(const int16_t* audio, int numSamples, int8_t* output);

// Global variables for preprocessing time
extern unsigned long last_preprocessing_time;

#endif
