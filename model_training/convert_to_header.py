import os
import json
import argparse

TARGET_CATEGORIES = {
    "dog": "Animals", "cat": "Animals", "cow": "Animals", "frog": "Animals",
    "rooster": "Animals", "pig": "Animals", "sheep": "Animals", "crow": "Animals",
    "rain": "Nature", "sea_waves": "Nature", "crackling_fire": "Nature", "wind": "Nature",
    "thunderstorm": "Nature", "pouring_water": "Nature",
    "clapping": "Human", "breathing": "Human", "coughing": "Human",
    "footsteps": "Human", "laughing": "Human", "snoring": "Human", "sneezing": "Human"
}

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--model", type=str, default="model_quantized.tflite")
    parser.add_argument("--labels", type=str, default="class_labels.json")
    args = parser.parse_args()

    if not os.path.exists(args.model):
        print(f"Error: Model file {args.model} not found.")
        return

    print(f"Reading {args.model}...")
    with open(args.model, "rb") as f:
        tflite_content = f.read()

    print("Generating model_data.h...")
    hex_array = [f"0x{b:02x}" for b in tflite_content]
    
    with open("model_data.h", "w") as f:
        f.write("#ifndef MODEL_DATA_H\n")
        f.write("#define MODEL_DATA_H\n\n")
        f.write(f"// Model size: {len(tflite_content)} bytes\n")
        f.write("alignas(16) const unsigned char model_tflite[] = {\n")
        
        # Write in chunks for readability
        chunk_size = 12
        for i in range(0, len(hex_array), chunk_size):
            chunk = ", ".join(hex_array[i:i+chunk_size])
            if i + chunk_size < len(hex_array):
                chunk += ","
            f.write(f"    {chunk}\n")
            
        f.write("};\n\n")
        f.write(f"const unsigned int model_tflite_len = {len(tflite_content)};\n\n")
        f.write("#endif // MODEL_DATA_H\n")

    print(f"Generating class_labels.h from {args.labels}...")
    if not os.path.exists(args.labels):
        print(f"Error: Labels file {args.labels} not found.")
        return
        
    with open(args.labels, "r") as f:
        labels_map = json.load(f)
        
    num_classes = len(labels_map)
    sorted_labels = [labels_map[str(i)] for i in range(num_classes)]
    sorted_cats = [TARGET_CATEGORIES.get(label, "Unknown") for label in sorted_labels]
    
    with open("class_labels.h", "w") as f:
        f.write("#ifndef CLASS_LABELS_H\n")
        f.write("#define CLASS_LABELS_H\n\n")
        f.write(f"const int NUM_CLASSES = {num_classes};\n\n")
        
        f.write("const char* CLASS_LABELS[] = {\n")
        for label in sorted_labels:
            f.write(f'    "{label}",\n')
        f.write("};\n\n")
        
        f.write("const char* CLASS_CATEGORIES[] = {\n")
        for cat in sorted_cats:
            f.write(f'    "{cat}",\n')
        f.write("};\n\n")
        
        f.write("#endif // CLASS_LABELS_H\n")

    print(f"Generated model_data.h ({os.path.getsize('model_data.h') / 1024:.2f} KB)")
    print(f"Generated class_labels.h ({os.path.getsize('class_labels.h') / 1024:.2f} KB)")

if __name__ == "__main__":
    main()
