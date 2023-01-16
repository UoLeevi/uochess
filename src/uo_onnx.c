#include "uo_onnx.h"

#include <stdlib.h>
#include <string.h>

uo_onnx_attribute *uo_onnx_helper_make_attribute(char *name, uo_onnx_attribute_type type, uo_onnx_attribute_value value)
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

uo_onnx_node *uo_onnx_helper_make_node(const char *op_type, const char **inputs, const char **outputs, const char *name, uo_onnx_attribute **attributes)
{

}
