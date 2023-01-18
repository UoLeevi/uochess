#ifndef UO_ONNX_HELPER_H
#define UO_ONNX_HELPER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "uo_onnx.h"

  uo_onnx_attribute *uo_onnx_make_attribute(char *name, uo_onnx_attribute_type type, uo_onnx_attribute_value value);

  uo_onnx_node *uo_onnx_make_node(const char *op_type, const char **inputs, const char **outputs, const char *name, uo_onnx_attribute **attributes);

#ifdef __cplusplus
}
#endif

#endif
