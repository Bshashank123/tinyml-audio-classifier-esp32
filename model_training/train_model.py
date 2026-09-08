import os
import json
import argparse
import numpy as np
import pandas as pd
import librosa
import tensorflow as tf
from sklearn.metrics import classification_report, confusion_matrix

ESC50_DIR = r"c:\Users\shash\OneDrive\Desktop\Anti\tiny\ESC-50-master\ESC-50-master"
META_CSV = os.path.join(ESC50_DIR, "meta", "esc50.csv")
AUDIO_DIR = os.path.join(ESC50_DIR, "audio")

# Core 5 Target Classes
TARGET_CLASSES = {
    0: "dog",
    5: "cat",
    10: "rain",
    22: "clapping",
    24: "coughing"
}

# 1.0s window for ultra-fast, high-temporal resolution
PREPROCESS_CONFIG = {
    "sample_rate": 16000,
    "n_mels": 32,
    "n_fft": 512,
    "hop_length": 500,
    "duration": 1.0,
    "expected_frames": 32
}

def extract_mel(y, sr):
    S = librosa.feature.melspectrogram(
        y=y,
        sr=sr,
        n_fft=PREPROCESS_CONFIG["n_fft"],
        hop_length=PREPROCESS_CONFIG["hop_length"],
        n_mels=PREPROCESS_CONFIG["n_mels"],
        center=False
    )
    # Natural log matching ESP32 implementation
    S_log = np.log(S + 1e-6)
    min_val = S_log.min()
    max_val = S_log.max()
    S_norm = (S_log - min_val) / (max_val - min_val + 1e-6)
    
    # Shape: (frames, mels, 1) -> (32, 32, 1)
    S_norm = S_norm.T
    if S_norm.shape[0] > PREPROCESS_CONFIG["expected_frames"]:
        S_norm = S_norm[:PREPROCESS_CONFIG["expected_frames"], :]
    elif S_norm.shape[0] < PREPROCESS_CONFIG["expected_frames"]:
        S_norm = np.pad(S_norm, ((0, PREPROCESS_CONFIG["expected_frames"] - S_norm.shape[0]), (0, 0)), "constant")
        
    return S_norm[..., np.newaxis].astype(np.float32)

def augment_audio(y, sr):
    augmented = []
    # 1. Original
    augmented.append(y)
    
    # 2. Random gain
    gain = np.random.uniform(0.6, 1.4)
    augmented.append(np.clip(y * gain, -1.0, 1.0))
    
    # 3. Additive noise
    noise_amp = 0.005 * np.random.uniform(0.5, 1.5)
    noise = noise_amp * np.random.randn(len(y))
    augmented.append(np.clip(y + noise, -1.0, 1.0))
    
    # 4. Time shift
    shift = int(np.random.uniform(-0.1, 0.1) * len(y))
    augmented.append(np.roll(y, shift))
    
    return augmented

def spec_augment(mel_spec, max_mask_pct=0.15):
    augmented = mel_spec.copy()
    n_frames, n_mels, _ = augmented.shape
    
    # Time mask
    t_mask_len = int(np.random.uniform(0, max_mask_pct * n_frames))
    if t_mask_len > 0:
        t0 = np.random.randint(0, n_frames - t_mask_len)
        augmented[t0:t0+t_mask_len, :, :] = 0.0
        
    # Freq mask
    f_mask_len = int(np.random.uniform(0, max_mask_pct * n_mels))
    if f_mask_len > 0:
        f0 = np.random.randint(0, n_mels - f_mask_len)
        augmented[:, f0:f0+f_mask_len, :] = 0.0
        
    return augmented

def load_dataset():
    df = pd.read_csv(META_CSV)
    target_ids = sorted(list(TARGET_CLASSES.keys()))
    df_filtered = df[df['target'].isin(target_ids)].copy()
    
    target_to_idx = {t: i for i, t in enumerate(target_ids)}
    idx_to_label = {str(i): TARGET_CLASSES[t] for i, t in enumerate(target_ids)}
    
    df_filtered['new_target'] = df_filtered['target'].map(target_to_idx)
    print(f"Loaded {len(df_filtered)} files across {len(target_ids)} classes.")
    
    X_train, y_train = [], []
    X_test, y_test = [], []
    
    window_samples = int(PREPROCESS_CONFIG["sample_rate"] * PREPROCESS_CONFIG["duration"]) # 16000
    stride_samples = int(PREPROCESS_CONFIG["sample_rate"] * 0.25) # 4000 (0.25s stride)
    
    for _, row in df_filtered.iterrows():
        filepath = os.path.join(AUDIO_DIR, row['filename'])
        y_audio, sr = librosa.load(filepath, sr=PREPROCESS_CONFIG["sample_rate"])
        fold = row['fold']
        target = row['new_target']
        
        # Sliding 1-second windows
        for start_idx in range(0, len(y_audio) - window_samples + 1, stride_samples):
            segment = y_audio[start_idx:start_idx + window_samples]
            
            # Skip near-silence
            if np.max(np.abs(segment)) < 0.01:
                continue
                
            if fold in [1, 2, 3, 4]: # 80% Train
                for aug_seg in augment_audio(segment, sr):
                    mel = extract_mel(aug_seg, sr)
                    X_train.append(mel)
                    y_train.append(target)
                    
                    X_train.append(spec_augment(mel))
                    y_train.append(target)
            else: # 20% Test (Fold 5)
                mel = extract_mel(segment, sr)
                X_test.append(mel)
                y_test.append(target)
                
    X_train = np.array(X_train, dtype=np.float32)
    y_train = np.array(y_train, dtype=np.int32)
    X_test = np.array(X_test, dtype=np.float32)
    y_test = np.array(y_test, dtype=np.int32)
    
    print(f"Total training samples (with augmentation): {X_train.shape[0]}")
    print(f"Total test samples: {X_test.shape[0]}")
    return X_train, y_train, X_test, y_test, idx_to_label

def build_model(num_classes):
    inputs = tf.keras.Input(shape=(32, 32, 1))
    
    # Block 1
    x = tf.keras.layers.Conv2D(16, (3, 3), padding='same', activation='relu')(inputs)
    x = tf.keras.layers.BatchNormalization()(x)
    x = tf.keras.layers.MaxPooling2D((2, 2))(x)
    
    # Block 2 - Depthwise Separable
    x = tf.keras.layers.SeparableConv2D(32, (3, 3), padding='same', activation='relu')(x)
    x = tf.keras.layers.BatchNormalization()(x)
    x = tf.keras.layers.MaxPooling2D((2, 2))(x)
    
    # Block 3 - Depthwise Separable
    x = tf.keras.layers.SeparableConv2D(48, (3, 3), padding='same', activation='relu')(x)
    x = tf.keras.layers.BatchNormalization()(x)
    x = tf.keras.layers.MaxPooling2D((2, 2))(x)
    
    # Classifier
    x = tf.keras.layers.GlobalAveragePooling2D()(x)
    x = tf.keras.layers.Dense(32, activation='relu')(x)
    x = tf.keras.layers.Dropout(0.3)(x)
    outputs = tf.keras.layers.Dense(num_classes, activation='softmax')(x)
    
    model = tf.keras.Model(inputs=inputs, outputs=outputs)
    return model

def main():
    print("="*50)
    print("   Training 5-Class High Accuracy TinyML Model   ")
    print("="*50)
    
    X_train, y_train, X_test, y_test, idx_to_label = load_dataset()
    
    indices = np.arange(len(X_train))
    np.random.seed(42)
    np.random.shuffle(indices)
    X_train, y_train = X_train[indices], y_train[indices]
    
    model = build_model(len(idx_to_label))
    model.compile(
        optimizer=tf.keras.optimizers.Adam(learning_rate=0.001),
        loss='sparse_categorical_crossentropy',
        metrics=['accuracy']
    )
    model.summary()
    
    callbacks = [
        tf.keras.callbacks.ReduceLROnPlateau(monitor='val_accuracy', factor=0.5, patience=5, min_lr=1e-5, verbose=1),
        tf.keras.callbacks.EarlyStopping(monitor='val_accuracy', patience=12, restore_best_weights=True, verbose=1)
    ]
    
    history = model.fit(
        X_train, y_train,
        validation_data=(X_test, y_test),
        epochs=50,
        batch_size=32,
        callbacks=callbacks
    )
    
    print("\nEvaluating on Test Set:")
    test_loss, test_acc = model.evaluate(X_test, y_test, verbose=0)
    print(f"===> Final Test Accuracy: {test_acc * 100:.2f}% <===")
    
    y_pred = np.argmax(model.predict(X_test), axis=1)
    print("\nClassification Report:")
    target_names = [idx_to_label[str(i)] for i in range(len(idx_to_label))]
    print(classification_report(y_test, y_pred, target_names=target_names))
    
    os.makedirs("model_training", exist_ok=True)
    model.save("model_training/model.keras")
    with open("model_training/class_labels.json", "w") as f:
        json.dump(idx_to_label, f, indent=2)
    with open("model_training/preprocess_config.json", "w") as f:
        json.dump(PREPROCESS_CONFIG, f, indent=2)
        
    np.save("model_training/X_train.npy", X_train)
    np.save("model_training/X_test.npy", X_test)
    np.save("model_training/y_test.npy", y_test)
    print("\nSaved trained model and assets.")

if __name__ == "__main__":
    main()

