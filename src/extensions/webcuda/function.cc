//#include <node_buffer.h>
#include <cstring>
#include <cstdio>
#include <iostream>
#include "function.h"
#include "mem.h"

using namespace webcuda;
using namespace v8;
using std::cout;
using std::endl;




Persistent<ObjectTemplate> webcuda::Function::constructor_template;

void webcuda::Function::Initialize(v8::Isolate* isolate, Handle<ObjectTemplate> webcuda_templ) {
  HandleScope scope(isolate);

	webcuda_templ->Set(String::NewFromUtf8(isolate, "launchKernel"),
			FunctionTemplate::New(isolate, LaunchKernel));

	webcuda_templ->Set(String::NewFromUtf8(isolate, "Function"),
			FunctionTemplate::New(isolate, MakeFunctionObject));

	Handle<ObjectTemplate> raw_template = MakeFunctionTemplate(isolate);
	constructor_template.Reset(isolate, raw_template);

  //target->Set(String::NewSymbol("Device"), constructor_template->GetFunction());
}

Handle<ObjectTemplate> webcuda::Function::MakeFunctionTemplate(Isolate* isolate) {
  EscapableHandleScope handle_scope(isolate);

  Local<ObjectTemplate> result = ObjectTemplate::New(isolate);
  result->SetInternalFieldCount(1);

  // Again, return the result through the current handle scope.
  return handle_scope.Escape(result);
}

//void MakeFunctionObject(const v8::FunctionCallbackInfo<v8::Value>& args) {
Handle<Object> webcuda::Function::MakeFunctionObject_(Isolate* isolate) {
	EscapableHandleScope handle_scope(isolate);

	//retrieving device information

	Function* fdevice = new Function();

	//creating object
	Handle<ObjectTemplate> templ = Local<ObjectTemplate>::New(isolate, constructor_template);
	Local<Object> result = templ->NewInstance();

	//wrap the raw C++ pointer in an External so it can be referenced from within JavaScript
	Handle<External> device_ptr = External::New(isolate, fdevice);

	//Store the pointer in the JavaScript Wrapper
	result->SetInternalField(0, device_ptr);

	return handle_scope.Escape(result);
}

webcuda::Function* webcuda::Function::UnwrapFunction(Handle<Object> obj) {
	Handle<External> field = Handle<External>::Cast(obj->GetInternalField(0));
	void* ptr = field->Value();
	return static_cast<Function*>(ptr);
}

void webcuda::Function::MakeFunctionObject(const v8::FunctionCallbackInfo<v8::Value>& args) {
	HandleScope handle_scope(args.GetIsolate());
	Handle<Object> temp = MakeFunctionObject_(args.GetIsolate());

	args.GetReturnValue().Set(temp);
}

/** \param args contains the block dimensions, thread dimensions, and the arguments for the kernel launch
 *
 * Function uses cuLaunchKernel)
 */
void webcuda::Function::LaunchKernel(const v8::FunctionCallbackInfo<v8::Value>& args) {
	HandleScope handle_scope(args.GetIsolate());
	Function* pfunction = UnwrapFunction(Handle<Object>::Cast(args[0]));

	Local<Array> gridDim = Local<Array>::Cast(args[1]);
	unsigned int gridDimX = gridDim->Get(0)->Uint32Value();
	unsigned int gridDimY = gridDim->Get(1)->Uint32Value();
	unsigned int gridDimZ = gridDim->Get(2)->Uint32Value();
	cout << gridDimX << "," << gridDimY << "," << gridDimZ << endl;

	Local<Array> blockDim = Local<Array>::Cast(args[2]);
	unsigned int blockDimX = blockDim->Get(0)->Uint32Value();
	unsigned int blockDimY = blockDim->Get(1)->Uint32Value();
	unsigned int blockDimZ = blockDim->Get(2)->Uint32Value();

	/*
	Local<Object> buf = args[3]->ToObject();
	//char *pbuffer = Buffer::Data(buf);
  char *pbuffer = static_cast<char*>(buf->GetIndexedPropertiesExternalArrayData());
	//size_t bufferSize = Buffer::Length(buf);
  size_t bufferSize =  buf->GetIndexedPropertiesExternalArrayDataLength();
	*/

	Handle<Object> mem = Handle<Object>::Cast(args[3]);
	size_t bufferSize;
	void *bufferLoc = Mem::GetDevicePtr(mem, &bufferSize);
	void *pbuffer = malloc(bufferSize);
	cout << bufferSize << endl;
	memcpy(pbuffer,bufferLoc,bufferSize);
	cout <<bufferLoc << ", " <<  pbuffer << ", " << endl;
	/*
	Handle<ArrayBuffer> buf = Handle<ArrayBuffer>::Cast(args[3]);
	if(buf->IsExternal()){
		cout << "yep, is external" << endl;
	}
	v8::ArrayBuffer::Contents ctx = buf->Externalize();
  //char *phost = static_cast<char*>(ctx.Data());
  void *pbuffer = ctx.Data();
	size_t bufferSize = ctx.ByteLength();
	*/

	/*
	void *kernelParams[] = {pbuffer};
	
	CUresult error = cuLaunchKernel(pfunction->m_function,
			gridDimX, gridDimY, gridDimZ,
			blockDimX, blockDimY, blockDimZ,
			0, 0, kernelParams, NULL);
			*/

	void *cuExtra[] = {
		CU_LAUNCH_PARAM_BUFFER_POINTER, pbuffer,
		CU_LAUNCH_PARAM_BUFFER_SIZE,    &bufferSize,
		CU_LAUNCH_PARAM_END
	};
	

	CUresult error = cuLaunchKernel(pfunction->m_function,
			gridDimX, gridDimY, gridDimZ,
			blockDimX, blockDimY, blockDimZ,
			0, 0, NULL, cuExtra);

	if(error == CUDA_ERROR_INVALID_VALUE){
		cout << "this is the error" << endl;
	}

	free(pbuffer);
	args.GetReturnValue().Set(Number::New(args.GetIsolate(), error));
}

