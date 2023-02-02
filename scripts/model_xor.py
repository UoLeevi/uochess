import tensorflow as tf

x = tf.placeholder(tf.float32, shape=[None, 2], name='input')
y_true = tf.placeholder(tf.float32, shape=[None, 1], name='target')

# Define the first dense layer
dense_1 = tf.layers.dense(inputs=x, units=64, activation=tf.nn.relu)

# Define the dropout layer
dropout = tf.layers.dropout(inputs=dense_1, rate=0.2)

# Define the output layer
y_pred = tf.layers.dense(inputs=dropout, units=10, activation=tf.nn.sigmoid)


# Optimize loss
loss = tf.reduce_mean(tf.square(y_pred - y_true), name='loss')
optimizer = tf.train.GradientDescentOptimizer(learning_rate=0.01)
train_op = optimizer.minimize(loss, name='train')

init = tf.global_variables_initializer()

# tf.train.Saver.__init__ adds operations to the graph to save
# and restore variables.
saver_def = tf.train.Saver().as_saver_def()

print('Run this operation to initialize variables     : ', init.name)
print('Run this operation for a train step            : ', train_op.name)
print('Feed this tensor to set the checkpoint filename: ', saver_def.filename_tensor_name)
print('Run this operation to save a checkpoint        : ', saver_def.save_tensor_name)
print('Run this operation to restore a checkpoint     : ', saver_def.restore_op_name)

# Write the graph out to a file.
with open('graph_xor.pb', 'w') as f:
  f.write(tf.get_default_graph().as_graph_def().SerializeToString())
