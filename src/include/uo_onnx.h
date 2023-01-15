#ifndef UO_ONNX_H
#define UO_ONNX_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>
#include <stdint.h>

  typedef struct uo_onnx_type uo_onnx_type;
  typedef struct uo_onnx_graph uo_onnx_graph;

#define uo_arrayof(type) \
struct \
{ \
  type *items;\
  size_t count;\
}

  // incorrect
  typedef enum uo_onnx_type_denotation
  {
    uo_onnx_MAP,
    uo_onnx_OPAQUE,
    uo_onnx_OPTIONAL,
    uo_onnx_SEQUENCE,
    uo_onnx_SPARSE_TENSOR,
    uo_onnx_TENSOR
  } uo_onnx_type_denotation;

  // OK
  typedef enum uo_onnx_tensor_elem_type
  {
    uo_onnx_UNDEFINED = 0,
    // Basic types.
    uo_onnx_FLOAT = 1,   // float
    uo_onnx_UINT8 = 2,   // uint8_t
    uo_onnx_INT8 = 3,    // int8_t
    uo_onnx_UINT16 = 4,  // uint16_t
    uo_onnx_INT16 = 5,   // int16_t
    uo_onnx_INT32 = 6,   // int32_t
    uo_onnx_INT64 = 7,   // int64_t
    uo_onnx_STRING = 8,  // string
    uo_onnx_BOOL = 9,    // bool

    uo_onnx_FLOAT16 = 10,
    uo_onnx_DOUBLE = 11,
    uo_onnx_UINT32 = 12,
    uo_onnx_UINT64 = 13,
    uo_onnx_COMPLEX64 = 14,
    uo_onnx_COMPLEX128 = 15,
    uo_onnx_BFLOAT16 = 16
  } uo_onnx_tensor_elem_type;

  typedef union uo_onnx_tensor_data
  {
    // For float and complex64 values
    // Complex64 tensors are encoded as a single array of floats,
    // with the real components appearing in odd numbered positions,
    // and the corresponding imaginary component appearing in the
    // subsequent even numbered position. (e.g., [1.0 + 2.0i, 3.0 + 4.0i]
    // is encoded as [1.0, 2.0, 3.0, 4.0]
    // When this field is present, the data_type field MUST be FLOAT or COMPLEX64.
    uo_arrayof(float) float_data;

    // For int32, uint8, int8, uint16, int16, bool, and float16 values
    // float16 values must be bit-wise converted to an uint16_t prior
    // to writing to the buffer.
    // When this field is present, the data_type field MUST be
    // INT32, INT16, INT8, UINT16, UINT8, BOOL, FLOAT16 or BFLOAT16
    uo_arrayof(int32_t) int32_data;

    // For strings.
    // Each element of string_data is a UTF-8 encoded Unicode
    // string. No trailing null, no leading BOM. The protobuf "string"
    // scalar type is not used to match ML community conventions.
    // When this field is present, the data_type field MUST be STRING
    uo_arrayof(char) string_data;

    // For int64.
    // When this field is present, the data_type field MUST be INT64
    uo_arrayof(int64_t) int64_data;

    // For double
    // Complex128 tensors are encoded as a single array of doubles,
    // with the real components appearing in odd numbered positions,
    // and the corresponding imaginary component appearing in the
    // subsequent even numbered position. (e.g., [1.0 + 2.0i, 3.0 + 4.0i]
    // is encoded as [1.0, 2.0, 3.0, 4.0]
    // When this field is present, the data_type field MUST be DOUBLE or COMPLEX128
    uo_arrayof(double) double_data;

    // For uint64 and uint32 values
    // When this field is present, the data_type field MUST be
    // UINT32 or UINT64
    uo_arrayof(uint64_t) uint64_data;
  } uo_onnx_tensor_data;

  // Defines a tensor shape. A dimension can be either an integer value
  // or a symbolic variable. A symbolic variable represents an unknown
  // dimension.
  typedef struct uo_onnx_tensor_shape
  {
    uo_arrayof(struct
    {
      size_t dim_value;
      char *dim_param;
      char *denotation;
    }) dim;
  } uo_onnx_tensor_shape;

  // OK
  typedef struct uo_onnx_tensor
  {
    // The shape of the tensor.
    uo_arrayof(size_t) dims;

    // The data type of the tensor.
    uo_onnx_tensor_elem_type data_type;

    struct
    {
      size_t begin;
      size_t end;
    } segment;

    uo_onnx_tensor_data data;

    char *name;
    char *doc_string;

  } uo_onnx_tensor;

  typedef struct uo_onnx_map_type
  {
    char *key_type;
    uo_onnx_type value_type;
  } uo_onnx_map_type;

  typedef struct uo_onnx_opaque_type
  {
    char *domain;
    char *name;
  } uo_onnx_opaque_type;

  typedef struct uo_onnx_optional_type
  {
    uo_onnx_tensor_elem_type elem_type;
  } uo_onnx_optional_type;

  typedef struct uo_onnx_sequence_type
  {
    uo_onnx_tensor_elem_type elem_type;
  } uo_onnx_sequence_type;

  // see: https://onnx.ai/onnx/api/classes.html#tensorshapeproto
  typedef struct uo_onnx_tensor_shape
  {
    size_t dim_count;
    size_t *sizes;
  } uo_onnx_tensor_shape;

  typedef struct uo_onnx_sparse_tensor_type
  {
    uo_onnx_tensor_elem_type elem_type;
    uo_onnx_tensor_shape shape;
  } uo_onnx_sparse_tensor_type;

  typedef struct uo_onnx_tensor_type
  {
    uo_onnx_tensor_elem_type elem_type;
    uo_onnx_tensor_shape shape;
  } uo_onnx_tensor_type;

  // see: https://onnx.ai/onnx/api/classes.html#typeproto
  typedef struct uo_onnx_type
  {
    char *denotation;
    union
    {
      uo_onnx_map_type map_type;
      uo_onnx_opaque_type opaque_type;
      uo_onnx_optional_type optional_type;
      uo_onnx_sequence_type sequence_type;
      uo_onnx_sparse_tensor_type sparse_tensor_type;
      uo_onnx_tensor_type tensor_type;
    };
  } uo_onnx_type;

  // see: https://onnx.ai/onnx/api/classes.html#valueinfoproto
  // OK
  typedef struct uo_onnx_valueinfo
  {
    char *name;
    char *doc_string;
    uo_onnx_type type;
  } uo_onnx_valueinfo;

  // OK
  typedef enum uo_onnx_attribute_type
  {
    uo_onnx_attribute_type_UNDEFINED = 0,
    uo_onnx_attribute_type_FLOAT = 1,
    uo_onnx_attribute_type_INT = 2,
    uo_onnx_attribute_type_STRING = 3,
    uo_onnx_attribute_type_TENSOR = 4,
    uo_onnx_attribute_type_GRAPH = 5,
    uo_onnx_attribute_type_SPARSE_TENSOR = 11,
    uo_onnx_attribute_type_TYPE_PROTO = 13,

    uo_onnx_attribute_type_FLOATS = 6,
    uo_onnx_attribute_type_INTS = 7,
    uo_onnx_attribute_type_STRINGS = 8,
    uo_onnx_attribute_type_TENSORS = 9,
    uo_onnx_attribute_type_GRAPHS = 10,
    uo_onnx_attribute_type_SPARSE_TENSORS = 12,
    uo_onnx_attribute_type_TYPE_PROTOS = 14
  } uo_onnx_attribute_type;

  typedef union uo_onnx_attribute_value
  {
    float f;
    uint64_t i;
    char *s;
    uo_onnx_tensor *t;
    uo_onnx_graph *g;
    uo_onnx_tensor *sparse_tensor;
    uo_onnx_type *tp;

    uo_arrayof(float) floats;
    uo_arrayof(uint64_t) ints;
    uo_arrayof(char *) strings;
    uo_arrayof(uo_onnx_tensor *) tensors;
    uo_arrayof(uo_onnx_graph *) graphs;
    uo_arrayof(uo_onnx_tensor *) sparse_tensors;
    uo_arrayof(uo_onnx_type *) type_protos;
  } uo_onnx_attribute_value;

  // see: https://onnx.ai/onnx/api/classes.html#attributeproto
  typedef struct uo_onnx_attribute
  {
    char *name;
    char *doc_string;
    char *ref_attr_name;
    size_t count;
    uo_onnx_attribute_type type;
    uo_onnx_attribute_value value;
  } uo_onnx_attribute;

  // see: https://onnx.ai/onnx/api/classes.html#nodeproto

  typedef struct uo_onnx_node uo_onnx_node;

  // OK
  typedef struct uo_onnx_node
  {
    char *name;
    char *doc_string;
    char *op_type;
    char *domain;
    uo_arrayof(uo_onnx_node *) inputs;
    uo_arrayof(uo_onnx_node *) outputs;
    uo_arrayof(uo_onnx_attribute *) attributes;
  } uo_onnx_node;

  // see: https://onnx.ai/onnx/api/classes.html#graphproto
  typedef struct uo_onnx_graph
  {
    char *name;
    char *doc_string;
    char *domain;
    //char *quantization_annotation;
    //char *value_info;

    //size_t sparse_initializer_count;
    //uo_onnx_node **sparse_initializers;

    uo_arrayof(uo_onnx_node *) inputs;
    uo_arrayof(uo_onnx_node *) initializers;
    uo_arrayof(uo_onnx_node *) outputs;

    // The nodes in the graph, sorted topologically.
    uo_arrayof(uo_onnx_node *) nodes;

  } uo_onnx_graph;

  uo_onnx_attribute *uo_onnx_helper_make_attribute(char *name, uo_onnx_attribute_type type, uo_onnx_attribute_value value);

  uo_onnx_node *uo_onnx_helper_make_node(const char *op_type, const char **inputs, const char **outputs, const char *name, uo_onnx_attribute **attributes);


#ifdef __cplusplus
}
#endif

#endif
