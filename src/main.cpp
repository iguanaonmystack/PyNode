#include <Python.h>
// #include <datetime.h>
#include <nan.h>
#include <string>
#include <time.h>

#include "helpers.h"

PyObject *pModule;

void Call(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
  PyObject *pFunc, *pValue;

  v8::String::Utf8Value functionName(args[0]);
  pFunc = PyObject_GetAttrString(pModule, *functionName);

  if (pFunc && PyCallable_Check(pFunc))
  {
    const int pythonArgsCount = Py_GetNumArguments(pFunc);
    // printf("this function has %d arguments\n", pythonArgsCount);
    const int passedArgsCount = args.Length() - 1;
    // printf("you passed %d arguments\n", passedArgsCount);

    // Check if the passed args length matches the python function args length
    if (passedArgsCount != pythonArgsCount)
    {
      char *error;
      size_t len = (size_t)snprintf(NULL, 0, "The function '%s' has %d arguments, %d were passed", *functionName, pythonArgsCount, passedArgsCount);
      error = (char *)malloc(len + 1);
      snprintf(error, len + 1, "The function '%s' has %d arguments, %d were passed", *functionName, pythonArgsCount, passedArgsCount);
      Nan::ThrowError(error);
      free(error);
      return;
    }

    PyObject *pArgs = BuildPyArgs(args);
    pValue = PyObject_CallObject(pFunc, pArgs);
    Py_DECREF(pArgs);

    // printf("return type : %s\n", pValue->ob_type->tp_name);

    if (pValue != NULL)
    {
      if (strcmp(pValue->ob_type->tp_name, "int") == 0)
      {
        double result = PyLong_AsDouble(pValue);
        args.GetReturnValue().Set(Nan::New(result));
      }
      else if (strcmp(pValue->ob_type->tp_name, "float") == 0)
      {
        double result = PyFloat_AsDouble(pValue);
        args.GetReturnValue().Set(Nan::New(result));
      }
      else if (strcmp(pValue->ob_type->tp_name, "bytes") == 0)
      {
        auto str = Nan::New(PyBytes_AsString(pValue)).ToLocalChecked();
        args.GetReturnValue().Set(str);
      }
      else if (strcmp(pValue->ob_type->tp_name, "str") == 0)
      {
        auto str = Nan::New(PyUnicode_AsUTF8(pValue)).ToLocalChecked();
        args.GetReturnValue().Set(str);
      }
      else if (strcmp(pValue->ob_type->tp_name, "bool") == 0)
      {
        bool b = PyObject_IsTrue(pValue);
        args.GetReturnValue().Set(Nan::New(b));
      }
      else if (strcmp(pValue->ob_type->tp_name, "list") == 0)
      {
        auto arr = BuildV8Array(pValue);
        args.GetReturnValue().Set(arr);
      }
      else if (strcmp(pValue->ob_type->tp_name, "dict") == 0)
      {
        auto obj = BuildV8Dict(pValue);
        args.GetReturnValue().Set(obj);
      }
      Py_DECREF(pValue);
    }
    // else
    // {
    //   Py_DECREF(pFunc);
    //   PyErr_Print();
    //   fprintf(stderr, "Call failed\n");
    //   Nan::ThrowError("Function call failed");
    // }
  }
  else
  {
    Py_DECREF(pFunc);
    PyErr_Print();
    Nan::ThrowError("Function call failed");
  }
}

void StartInterpreter(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
  if (args.Length() == 1 && args[0]->IsString()) {
    v8::String::Utf8Value pathString(args[0]);
    std::wstring path(pathString.length(), L'#');
    mbstowcs(&path[0], *pathString, pathString.length());
    Py_SetPath(path.c_str());
  }

  Py_Initialize();
}

void AppendSysPath(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
  if (args.Length() == 0 || !args[0]->IsString()) {
    Nan::ThrowError("Must pass a string to 'appendSysPath'");
    return;
  }

  v8::String::Utf8Value pathName(args[0]);

  char *appendPathStr;
  size_t len = (size_t)snprintf(NULL, 0, "import sys;sys.path.append(\"%s\")", *pathName);
  appendPathStr = (char *)malloc(len + 1);
  snprintf(appendPathStr, len + 1, "import sys;sys.path.append(\"%s\")", *pathName);

  PyRun_SimpleString(appendPathStr);
  free(appendPathStr);
}

void OpenFile(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
  if (args.Length() == 0 || !args[0]->IsString()) {
    Nan::ThrowError("Must pass a string to 'openFile'");
    return;
  }

  v8::String::Utf8Value fileName(args[0]);

  PyObject *pName;
  pName = PyUnicode_DecodeFSDefault(*fileName);

  pModule = PyImport_Import(pName);
  Py_DECREF(pName);

  if (pModule == NULL)
  {
    PyErr_Print();
    fprintf(stderr, "Failed to load module: %s\n", *fileName);
    Nan::ThrowError("Failed to load python module");
    return;
  }
}

void Initialize(v8::Local<v8::Object> exports)
{
  exports->Set(
      Nan::New("call").ToLocalChecked(),
      Nan::New<v8::FunctionTemplate>(Call)->GetFunction());

  exports->Set(
      Nan::New("startInterpreter").ToLocalChecked(),
      Nan::New<v8::FunctionTemplate>(StartInterpreter)->GetFunction());

  exports->Set(
      Nan::New("appendSysPath").ToLocalChecked(),
      Nan::New<v8::FunctionTemplate>(AppendSysPath)->GetFunction());

  exports->Set(
      Nan::New("openFile").ToLocalChecked(),
      Nan::New<v8::FunctionTemplate>(OpenFile)->GetFunction());
}

NODE_MODULE(addon, Initialize);