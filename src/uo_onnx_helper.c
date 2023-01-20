#include "uo_onnx_helper.h"

#include <stdlib.h>
#include <string.h>

uo_onnx_attribute *uo_onnx_make_attribute(char *name, uo_onnx_attribute_type type, uo_onnx_attribute_value value)
{
  uo_onnx_attribute *attribute;
  size_t name_len = strlen(name);

  switch (type)
  {
    case uo_onnx_attribute_type_FLOAT:
    case uo_onnx_attribute_type_INT:
    case uo_onnx_attribute_type_STRING:
    case uo_onnx_attribute_type_TENSOR:
    case uo_onnx_attribute_type_GRAPH:
    case uo_onnx_attribute_type_SPARSE_TENSOR:
    case uo_onnx_attribute_type_TYPE_PROTO:
      attribute = calloc(1, name_len + 1 + sizeof * attribute);
      memcpy(attribute->name, name, name_len);
      attribute->type = type;
      attribute->value = value;
      return attribute;

    case uo_onnx_attribute_type_FLOATS:
      attribute = calloc(1, name_len + 1 + value.floats.count * sizeof * value.floats.items + sizeof * attribute);
      memcpy(attribute->name, name, name_len);
      attribute->type = type;
      attribute->value.floats.count = value.floats.count;
      memcpy(attribute->value.floats.items, value.floats.items, value.floats.count * sizeof * value.floats.items);
      return attribute;

    case uo_onnx_attribute_type_INTS:
      attribute = calloc(1, name_len + 1 + value.ints.count * sizeof * value.ints.items + sizeof * attribute);
      memcpy(attribute->name, name, name_len);
      attribute->type = type;
      attribute->value.ints.count = value.ints.count;
      memcpy(attribute->value.ints.items, value.ints.items, value.ints.count * sizeof * value.ints.items);
      return attribute;

    case uo_onnx_attribute_type_STRINGS:
      attribute = calloc(1, name_len + 1 + value.strings.count * sizeof * value.strings.items + sizeof * attribute);
      memcpy(attribute->name, name, name_len);
      attribute->type = type;
      attribute->value.strings.count = value.strings.count;
      memcpy(attribute->value.strings.items, value.strings.items, value.strings.count * sizeof * value.strings.items);
      return attribute;

    case uo_onnx_attribute_type_TENSORS:
      attribute = calloc(1, name_len + 1 + value.tensors.count * sizeof * value.tensors.items + sizeof * attribute);
      memcpy(attribute->name, name, name_len);
      attribute->type = type;
      attribute->value.tensors.count = value.tensors.count;
      memcpy(attribute->value.tensors.items, value.tensors.items, value.tensors.count * sizeof * value.tensors.items);
      return attribute;

    case uo_onnx_attribute_type_GRAPHS:
      attribute = calloc(1, name_len + 1 + value.graphs.count * sizeof * value.graphs.items + sizeof * attribute);
      memcpy(attribute->name, name, name_len);
      attribute->type = type;
      attribute->value.graphs.count = value.graphs.count;
      memcpy(attribute->value.graphs.items, value.graphs.items, value.graphs.count * sizeof * value.graphs.items);
      return attribute;

    case uo_onnx_attribute_type_SPARSE_TENSORS:
      attribute = calloc(1, name_len + 1 + value.sparse_tensors.count * sizeof * value.sparse_tensors.items + sizeof * attribute);
      memcpy(attribute->name, name, name_len);
      attribute->type = type;
      attribute->value.sparse_tensors.count = value.sparse_tensors.count;
      memcpy(attribute->value.sparse_tensors.items, value.sparse_tensors.items, value.sparse_tensors.count * sizeof * value.sparse_tensors.items);
      return attribute;

    case uo_onnx_attribute_type_TYPE_PROTOS:
      attribute = calloc(1, name_len + 1 + value.type_protos.count * sizeof * value.type_protos.items + sizeof * attribute);
      memcpy(attribute->name, name, name_len);
      attribute->type = type;
      attribute->value.type_protos.count = value.type_protos.count;
      memcpy(attribute->value.type_protos.items, value.type_protos.items, value.type_protos.count * sizeof * value.type_protos.items);
      return attribute;

    default:
      return NULL;
  }
}

uo_onnx_node *uo_onnx_make_node(const char *op_type, const char **inputs, const char **outputs, const char *name, uo_onnx_attribute **attributes)
{
  size_t input_count = 0;
  while (inputs[input_count])
  {
    ++input_count;
  }
  size_t inputs_size = sizeof(char *) * input_count;

  size_t output_count = 0;
  while (outputs[output_count])
  {
    ++output_count;
  }
  size_t outputs_size = sizeof(char *) * output_count;

  size_t attribute_count = 0;
  while (attributes[attribute_count])
  {
    ++attribute_count;
  }
  size_t attributes_size = sizeof(uo_onnx_attribute *) * attribute_count;

  size_t op_type_len = strlen(op_type);
  size_t name_len = strlen(name);
  char *mem = calloc(1, sizeof(uo_onnx_node) + inputs_size + outputs_size + attributes_size + op_type_len + 1 + name_len + 1);

  uo_onnx_node *node = (void *)mem;

  mem += sizeof(uo_onnx_node);
  node->inputs.items = mem;
  node->inputs.count = input_count;
  memcpy(mem, inputs, inputs_size);

  mem += inputs_size;
  node->outputs.items = mem;
  node->outputs.count = output_count;
  memcpy(mem, outputs, outputs_size);

  mem += outputs_size;
  node->attributes.items = mem;
  node->attributes.count = attribute_count;
  memcpy(mem, attributes, attributes_size);

  mem += attributes_size;
  node->op_type = mem;
  memcpy(mem, op_type, op_type_len);

  mem += op_type_len + 1;
  node->name = mem;
  memcpy(mem, name, name_len);

  return node;
}

uo_onnx_graph *uo_onnx_make_graph(uo_onnx_node **nodes, const char *name, uo_onnx_valueinfo **inputs, uo_onnx_valueinfo **outputs, uo_onnx_tensor **initializers)
{
  size_t node_count = 0;
  while (nodes[node_count])
  {
    ++node_count;
  }
  size_t nodes_size = sizeof(uo_onnx_node *) * node_count;

  size_t input_count = 0;
  while (inputs[input_count])
  {
    ++input_count;
  }
  size_t inputs_size = sizeof(uo_onnx_valueinfo *) * input_count;

  size_t output_count = 0;
  while (outputs[output_count])
  {
    ++output_count;
  }
  size_t outputs_size = sizeof(uo_onnx_valueinfo *) * output_count;

  size_t initializer_count = 0;
  while (initializers[initializer_count])
  {
    ++initializer_count;
  }
  size_t initializers_size = sizeof(uo_onnx_valueinfo *) * initializer_count;

  size_t name_len = strlen(name);
  char *mem = calloc(1, sizeof(uo_onnx_graph) + nodes_size + inputs_size + outputs_size + initializers_size + name_len + 1);

  uo_onnx_graph *graph = (void *)mem;

  mem += sizeof(uo_onnx_node);
  graph->nodes.items = mem;
  graph->nodes.count = input_count;
  memcpy(mem, nodes, inputs_size);

  graph->inputs.items = mem;
  graph->inputs.count = input_count;
  memcpy(mem, inputs, inputs_size);

  mem += inputs_size;
  graph->outputs.items = mem;
  graph->outputs.count = output_count;
  memcpy(mem, outputs, outputs_size);

  mem += outputs_size;
  graph->initializers.items = mem;
  graph->initializers.count = initializer_count;
  memcpy(mem, initializers, initializers_size);

  mem += initializers_size;

  graph->name = mem;
  memcpy(mem, name, name_len);

  // TODO:
  // - topologically sort nodes and make graph structure to be better suitable for execution

  return graph;
}
