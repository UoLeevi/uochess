import tensorflow as tf

# Define the input layer
input_layer = tf.keras.layers.Input(shape=(2,))

# Define the first dense layer
dense_1 = tf.keras.layers.Dense(32, activation='relu')(input_layer)

# Define the second dense layer
dense_2 = tf.keras.layers.Dense(32, activation='relu')(dense_1)

# Define the output layer
output_layer = tf.keras.layers.Dense(1, activation='sigmoid')(dense_2)

# Define the model
model = tf.keras.models.Model(inputs=input_layer, outputs=output_layer)

# Compile the model
model.compile(loss='binary_crossentropy', optimizer='adam', metrics=['accuracy'])

# Save the model as a TensorFlow SavedModel
tf.keras.models.save_model(model, 'src/nn/model_xor')

