#include <node.h>
#include <node_buffer.h>
#include <string.h>
#include <stdlib.h>

#include <unicode/ucnv.h>
#include <unicode/ucsdet.h>

using namespace node;
using namespace v8;

struct Baton {
    uv_work_t request;
    Persistent<Function> callback;

    char* data;
    uint32_t dataLen;

    bool error;
    const char* error_message;

    const char* result;
};


static void DetectWork(uv_work_t* req) {
    Baton* baton = static_cast<Baton*>(req->data);

    UErrorCode status = U_ZERO_ERROR;
    UCharsetDetector* detector = ucsdet_open(&status);
    if (U_FAILURE(status)) {
        baton->error_message = "Could not open detector";
        baton->error = true;
        return;
    }

    ucsdet_enableInputFilter(detector, true);

    ucsdet_setText(detector, baton->data, baton->dataLen, &status);

    if (U_FAILURE(status)) {
        baton->error_message = "Could not set text";
        baton->error = true;
        return;
    }

    const UCharsetMatch* match = ucsdet_detect(detector, &status);
    if (U_FAILURE(status)) {
        baton->error_message = "Could not detect";
        baton->error = true;

        ucsdet_close(detector);
        return;
    }

    const char* matchEncoding = ucsdet_getName(match, &status);
    if (U_FAILURE(status)) {
        baton->error_message = "Could not get encoding name";
        baton->error = true;

        ucsdet_close(detector);
        return;
    }

    baton->result = matchEncoding;

    ucsdet_close(detector);
}

static void DetectAfter(uv_work_t* req) {
    HandleScope scope;
    Baton* baton = static_cast<Baton*>(req->data);

    if (baton->error) {
        Local<Value> err = Exception::Error(String::New(baton->error_message));

        const unsigned argc = 1;
        Local<Value> argv[argc] = { err };

        TryCatch try_catch;
        baton->callback->Call(Context::GetCurrent()->Global(), argc, argv);
        if (try_catch.HasCaught())
            FatalException(try_catch);
    } else {
        const unsigned argc = 2;
        Local<Value> argv[argc] = {
            Local<Value>::New(Null()),
            Local<Value>::New(baton->result
                            ? String::New(baton->result)
                            : String::Empty())
        };

        TryCatch try_catch;
        baton->callback->Call(Context::GetCurrent()->Global(), argc, argv);
        if (try_catch.HasCaught())
            FatalException(try_catch);
    }

    baton->callback.Dispose();
    delete baton;
}

static Handle<Value> DetectTextEncoding(const Arguments& args) {
    if (args.Length() < 2) {
        return ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
    }

    if (!node::Buffer::HasInstance(args[0]) || !args[1]->IsFunction()) {
        return ThrowException(Exception::TypeError(String::New("Wrong arguments")));
    }

    Local<Function> callback = Local<Function>::Cast(args[1]);
    #if NODE_MAJOR_VERSION == 0 && NODE_MINOR_VERSION < 10
          Local<Object> buffer_obj = args[0]->ToObject();
    #else
          Local<Value> buffer_obj = args[0];
    #endif

    Baton* baton = new Baton();
    baton->error = false;
    baton->error_message = NULL;
    baton->request.data = baton;
    baton->callback = Persistent<Function>::New(callback);
    baton->data = Buffer::Data(buffer_obj);
    baton->dataLen = Buffer::Length(buffer_obj);
    baton->result = NULL;

    int status = uv_queue_work(uv_default_loop(),
                                     &baton->request,
                                     DetectWork,
                                     (uv_after_work_cb)DetectAfter);
    assert(status == 0);

    return Undefined();
}


extern "C" {
  void init(Handle<Object> exports) {
    exports->Set(String::NewSymbol("detectEncoding"),
          FunctionTemplate::New(DetectTextEncoding)->GetFunction());
  }

  NODE_MODULE(detect_encoding, init);
}
