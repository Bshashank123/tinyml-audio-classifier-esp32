#ifndef ML_INFERENCE_H
#define ML_INFERENCE_H

#include <stdint.h>

bool setupModel();
bool runInference(const int8_t* features, float* output_probs, int* predicted_class, float* confidence);

// Global variable for inference time
extern unsigned long last_inference_time;

#endif
