#include "ml_inference.h"
#include <Arduino.h>

// TensorFlow Lite Micro headers (from TensorFlowLite_ESP32 library)
#include <TensorFlowLite_ESP32.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"

#ifndef TFLITE_SCHEMA_VERSION
  #define TFLITE_SCHEMA_VERSION (3)
#endif
#include "model_data.h"
#include "class_labels.h"

unsigned long last_inference_time = 0;

// TensorFlow Lite globals
namespace {
    tflite::ErrorReporter* error_reporter = nullptr;
    const tflite::Model* model = nullptr;
    tflite::MicroInterpreter* interpreter = nullptr;
    TfLiteTensor* input = nullptr;
    TfLiteTensor* output = nullptr;

    // Tensor arena size: 60KB (sufficient for activations and state)
    constexpr int kTensorArenaSize = 60 * 1024;
    static uint8_t* tensor_arena = nullptr;
}

bool setupModel() {
    Serial.println("Setting up ML Model...");

    if (tensor_arena == nullptr) {
        if (psramFound()) {
            tensor_arena = (uint8_t*)ps_malloc(kTensorArenaSize);
        }
        if (tensor_arena == nullptr) {
            tensor_arena = (uint8_t*)malloc(kTensorArenaSize);
        }
        if (tensor_arena == nullptr) {
            Serial.println("ERROR: Failed to allocate tensor arena!");
            return false;
        }
    }

    static tflite::MicroErrorReporter micro_error_reporter;
    error_reporter = &micro_error_reporter;

    model = tflite::GetModel(model_tflite);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        Serial.printf("Model provided is schema version %d not equal to supported version %d.\n",
                      model->version(), TFLITE_SCHEMA_VERSION);
        return false;
    }

    // Register operations
    static tflite::AllOpsResolver resolver;

    // Build interpreter
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
    interpreter = &static_interpreter;

    // Allocate memory from the tensor_arena for the model's tensors.
    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk) {
        Serial.println("AllocateTensors() failed");
        return false;
    }

    // Get pointers to the model's input and output tensors
    input = interpreter->input(0);
    output = interpreter->output(0);
    
    Serial.printf("Model input shape: ");
    for(int i=0; i<input->dims->size; i++) {
        Serial.printf("%d ", input->dims->data[i]);
    }
    Serial.println();
    
    Serial.printf("Model output shape: ");
    for(int i=0; i<output->dims->size; i++) {
        Serial.printf("%d ", output->dims->data[i]);
    }
    Serial.println();
    
    Serial.printf("Arena used: %d bytes\n", interpreter->arena_used_bytes());

    return true;
}

bool runInference(const int8_t* features, float* output_probs, int* predicted_class, float* confidence) {
    unsigned long start_time = millis();
    
    // Copy features to input tensor
    // Assuming input is [1, 32, 64, 1] int8 tensor
    int input_length = input->bytes;
    for (int i = 0; i < input_length; i++) {
        input->data.int8[i] = features[i];
    }
    
    // Run inference
    TfLiteStatus invoke_status = interpreter->Invoke();
    if (invoke_status != kTfLiteOk) {
        Serial.println("Invoke failed");
        return false;
    }
    
    // Read outputs (assuming output is int8 quantized or float32)
    int max_index = 0;
    float max_score = -1.0f;
    
    // Handle both float and int8 output formats
    for (int i = 0; i < NUM_CLASSES; i++) {
        float val = 0.0f;
        if (output->type == kTfLiteInt8) {
            val = (output->data.int8[i] - output->params.zero_point) * output->params.scale;
        } else if (output->type == kTfLiteFloat32) {
            val = output->data.f[i];
        } else if (output->type == kTfLiteUInt8) {
             val = (output->data.uint8[i] - output->params.zero_point) * output->params.scale;
        }
        
        output_probs[i] = val;
        
        if (val > max_score) {
            max_score = val;
            max_index = i;
        }
    }
    
    *predicted_class = max_index;
    *confidence = max_score;
    
    last_inference_time = millis() - start_time;
    return true;
}
