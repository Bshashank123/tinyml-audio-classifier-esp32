#include "audio_processor.h"
#include <Arduino.h>
#include <math.h>
#include "arduinoFFT.h"

unsigned long last_preprocessing_time = 0;

static float vReal[N_FFT];
static float vImag[N_FFT];
static ArduinoFFT<float> FFT = ArduinoFFT<float>(vReal, vImag, N_FFT, SAMPLE_RATE);
static float melSpectrogram[N_FRAMES][N_MELS];

// Simplified Mel frequency conversion
static float hzToMel(float hz) {
    return 2595.0f * log10(1.0f + hz / 700.0f);
}

static float melToHz(float mel) {
    return 700.0f * (pow(10.0f, mel / 2595.0f) - 1.0f);
}

// Memory-optimized Mel filterbank bounds
static int melBandStart[N_MELS];
static int melBandCenter[N_MELS];
static int melBandEnd[N_MELS];

void setupAudioProcessor() {
    float melMin = hzToMel(0);
    float melMax = hzToMel(SAMPLE_RATE / 2);
    float melPoints[N_MELS + 2];
    float hzPoints[N_MELS + 2];
    int binPoints[N_MELS + 2];

    for (int i = 0; i < N_MELS + 2; i++) {
        melPoints[i] = melMin + i * (melMax - melMin) / (N_MELS + 1);
        hzPoints[i] = melToHz(melPoints[i]);
        binPoints[i] = floor((N_FFT + 1) * hzPoints[i] / SAMPLE_RATE);
    }

    for (int m = 1; m <= N_MELS; m++) {
        melBandStart[m - 1] = binPoints[m - 1];
        melBandCenter[m - 1] = binPoints[m];
        melBandEnd[m - 1] = binPoints[m + 1];
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
                float multiplier = 0.5f * (1.0f - cos(2.0f * M_PI * i / (N_FFT - 1)));
                vReal[i] = audio[startSample + i] * multiplier;
            } else {
                vReal[i] = 0.0f;
            }
            vImag[i] = 0.0f;
        }

        FFT.compute(FFTDirection::Forward);
        FFT.complexToMagnitude();

        for (int m = 0; m < N_MELS; m++) {
            float energy = 0.0f;
            for (int k = melBandStart[m]; k < melBandCenter[m]; k++) {
                if (k < N_FFT / 2) {
                    float weight = (float)(k - melBandStart[m]) / (melBandCenter[m] - melBandStart[m]);
                    energy += vReal[k] * vReal[k] * weight;
                }
            }
            for (int k = melBandCenter[m]; k <= melBandEnd[m]; k++) {
                if (k < N_FFT / 2) {
                    float weight = (float)(melBandEnd[m] - k) / (melBandEnd[m] - melBandCenter[m]);
                    energy += vReal[k] * vReal[k] * weight;
                }
            }
            
            // Log scale
            float log_energy = log(energy + 1e-6f);
            melSpectrogram[frame][m] = log_energy;
            if (log_energy > max_val) max_val = log_energy;
            if (log_energy < min_val) min_val = log_energy;
        }
    }

    // Normalize and quantize to int8 [-128, 127]
    for (int frame = 0; frame < N_FRAMES; frame++) {
        for (int m = 0; m < N_MELS; m++) {
            float norm = (melSpectrogram[frame][m] - min_val) / (max_val - min_val);
            output[frame * N_MELS + m] = (int8_t)((norm * 255.0f) - 128);
        }
    }
    
    last_preprocessing_time = millis() - start_time;
}
