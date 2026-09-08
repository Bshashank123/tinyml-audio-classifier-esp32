import numpy as np
import tensorflow as tf
import os
import argparse

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--model", type=str, default="model.keras")
    args = parser.parse_args()

    if not os.path.exists(args.model):
        print(f"Error: Model file {args.model} not found.")
        return

    print(f"Loading Keras model from {args.model}...")
    model = tf.keras.models.load_model(args.model)
    model_dir = os.path.dirname(args.model) or "."
    
    x_train_path = os.path.join(model_dir, "X_train.npy") if os.path.exists(os.path.join(model_dir, "X_train.npy")) else "X_train.npy"
    if not os.path.exists(x_train_path):
        print(f"Error: {x_train_path} not found for representative dataset.")
        return
        
    print(f"Loading representative dataset from {x_train_path}...")
    X_train = np.load(x_train_path)
    
    def representative_data_gen():
        for i in range(min(500, len(X_train))):
            yield [X_train[i:i+1].astype(np.float32)]

    out_float = os.path.join(model_dir, "model_float.tflite")
    out_quant = os.path.join(model_dir, "model_quantized.tflite")

    print("Exporting Float32 TFLite model...")
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    tflite_float_model = converter.convert()
    with open(out_float, "wb") as f:
        f.write(tflite_float_model)

    print("Exporting INT8 Quantized TFLite model...")
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    converter.optimizations = [tf.lite.Optimize.DEFAULT]
    converter.representative_dataset = representative_data_gen
    converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
    converter.inference_input_type = tf.int8
    converter.inference_output_type = tf.int8
    tflite_quant_model = converter.convert()
    with open(out_quant, "wb") as f:
        f.write(tflite_quant_model)

    float_size = os.path.getsize(out_float) / 1024
    quant_size = os.path.getsize(out_quant) / 1024
    print(f"Float32 model size: {float_size:.2f} KB")
    print(f"Quantized INT8 model size: {quant_size:.2f} KB")

    print("Evaluating quantized model on test set...")
    x_test_path = os.path.join(model_dir, "X_test.npy")
    y_test_path = os.path.join(model_dir, "y_test.npy")
    if not os.path.exists(x_test_path) or not os.path.exists(y_test_path):
        print("Test data not found, skipping evaluation.")
        return
        
    interpreter = tf.lite.Interpreter(model_content=tflite_quant_model)
    interpreter.allocate_tensors()
    input_details = interpreter.get_input_details()
    output_details = interpreter.get_output_details()
    
    input_scale, input_zero_point = input_details[0]['quantization']
    output_scale, output_zero_point = output_details[0]['quantization']
    
    X_test = np.load(x_test_path)
    y_test = np.load(y_test_path)
    
    correct = 0
    for i in range(len(X_test)):
        # Quantize input
        x_quant = (X_test[i:i+1] / input_scale + input_zero_point).astype(input_details[0]['dtype'])
        interpreter.set_tensor(input_details[0]['index'], x_quant)
        interpreter.invoke()
        y_quant = interpreter.get_tensor(output_details[0]['index'])
        # Dequantize not necessary for argmax
        pred = np.argmax(y_quant[0])
        if pred == y_test[i]:
            correct += 1
            
    print(f"Quantized model test accuracy: {correct / len(X_test):.4f}")

if __name__ == "__main__":
    main()
