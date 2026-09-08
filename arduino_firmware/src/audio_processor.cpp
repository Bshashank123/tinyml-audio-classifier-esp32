#include "audio_processor.h"
#include <Arduino.h>
#include <math.h>
#include "arduinoFFT.h"

#define N_FFT 1024
#define HOP_LENGTH 512
#define N_MELS 64
#define SR 16000
#define N_FRAMES 32

unsigned long last_preprocessing_time = 0;

static double vReal[N_FFT];
static double vImag[N_FFT];
static arduinoFFT FFT = arduinoFFT(vReal, vImag, N_FFT, SR);
static float melSpectrogram[N_FRAMES][N_MELS];

// Simplified Mel frequency conversion
static float hzToMel(float hz) {
    return 2595.0f * log10(1.0f + hz / 700.0f);
}

static float melToHz(float mel) {
    return 700.0f * (pow(10.0f, mel / 2595.0f) - 1.0f);
}

// Sparse Mel filterbank
// To save memory, we can compute weights on the fly or pre-calculate only bounds
static int melBandStart[N_MELS];
static int melBandEnd[N_MELS];
static float melWeights[513][N_MELS]; 

void setupAudioProcessor() {
    float melMin = hzToMel(0);
    float melMax = hzToMel(SR / 2);
    float melPoints[N_MELS + 2];
    float hzPoints[N_MELS + 2];
    int binPoints[N_MELS + 2];

    for (int i = 0; i < N_MELS + 2; i++) {
        melPoints[i] = melMin + i * (melMax - melMin) / (N_MELS + 1);
        hzPoints[i] = melToHz(melPoints[i]);
        binPoints[i] = floor((N_FFT + 1) * hzPoints[i] / SR);
    }

    memset(melWeights, 0, sizeof(melWeights));
    for (int m = 1; m <= N_MELS; m++) {
        melBandStart[m - 1] = binPoints[m - 1];
        melBandEnd[m - 1] = binPoints[m + 1];
        for (int k = binPoints[m - 1]; k < binPoints[m]; k++) {
            melWeights[k][m - 1] = (float)(k - binPoints[m - 1]) / (binPoints[m] - binPoints[m - 1]);
        }
        for (int k = binPoints[m]; k < binPoints[m + 1]; k++) {
            melWeights[k][m - 1] = (float)(binPoints[m + 1] - k) / (binPoints[m + 1] - binPoints[m]);
        }
    }
}

void computeMelSpectrogram(const int16_t* audio, int numSamples, int8_t* output) {
    unsigned long start_time = millis();
    float max_val = -1e9;
    float min_val = 1e9;

    for (int frame = 0; frame < N_FRAMES; frame++) {
        int startSample = frame * HOP_LENGTH;
        for (int i = 0; i < N_FFT; i++) {
            if (startSample + i < numSamples) {
                // Apply Hanning window
                float multiplier = 0.5 * (1 - cos(2 * M_PI * i / (N_FFT - 1)));
                vReal[i] = audio[startSample + i] * multiplier;
            } else {
                vReal[i] = 0.0;
            }
            vImag[i] = 0.0;
        }

        FFT.Windowing(FFT_WIN_TYP_HANNING, FFT_FORWARD);
        FFT.Compute(FFT_FORWARD);
        FFT.ComplexToMagnitude();

        for (int m = 0; m < N_MELS; m++) {
            float energy = 0.0f;
            for (int k = melBandStart[m]; k <= melBandEnd[m]; k++) {
                if (k < N_FFT / 2) {
                    energy += vReal[k] * vReal[k] * melWeights[k][m];
                }
            }
            // Log scale
            float log_energy = log(energy + 1e-6);
            melSpectrogram[frame][m] = log_energy;
            if (log_energy > max_val) max_val = log_energy;
            if (log_energy < min_val) min_val = log_energy;
        }
    }

    // Normalize and quantize to int8 [-128, 127]
    for (int frame = 0; frame < N_FRAMES; frame++) {
        for (int m = 0; m < N_MELS; m++) {
            float norm = (melSpectrogram[frame][m] - min_val) / (max_val - min_val);
            output[frame * N_MELS + m] = (int8_t)((norm * 255.0) - 128);
        }
    }
    
    last_preprocessing_time = millis() - start_time;
}
