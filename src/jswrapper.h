#ifndef PYNODE_JSWRAPPER_HPP
#define PYNODE_JSWRAPPER_HPP

#include <Python.h>
#include <node_api.h>

#ifdef __cplusplus
extern "C" {
#endif

PyMODINIT_FUNC PyInit_jswrapper(void);
PyObject *WrappedJSObject_New(napi_env, napi_value);

#ifdef __cplusplus
}
#endif

#endif
