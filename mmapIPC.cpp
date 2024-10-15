/*
* @Ref <br>
* 1- https://github.com/nodejs/node-addon-api/blob/main/doc/object_wrap.md <br>
* 2- https://github.com/nodejs/node-addon-api/blob/main/doc/object.md
* 3- https://github.com/nodejs/node-addon-api/blob/main/doc/class_property_descriptor.md
*/

#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/file.h>        /* For flock(2)*/
//#include <cstdio>
#include <napi.h>


//-----------------------CLASS----------------------//
class shm_metadata
{
  public:
    std::size_t mmapLength;
    std::string shm_fileName;
    int shm_fd;
};


class mmapIPC : public Napi::ObjectWrap<mmapIPC>
{
  public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);  //These three functions are required by Node!
    mmapIPC(const Napi::CallbackInfo& info);
    static Napi::Value CreateNewItem(const Napi::CallbackInfo& info);

  private:
    //-----------members
    shm_metadata shm_hint;
    Napi::Reference<Napi::Buffer<uint8_t>> memoryBuffer;
    //-----------funcs
    Napi::Value buffer(const Napi::CallbackInfo& info);
    Napi::Value acquireReadLock(const Napi::CallbackInfo& info);
    Napi::Value acquireWriteLock(const Napi::CallbackInfo& info);
    Napi::Value removeLock(const Napi::CallbackInfo& info);
    //-----------statics
    static void freeCallback(Napi::Env, void* memoryAddress, shm_metadata* hint);
    static int createShmFile( const char *name, std::size_t memLenght);
    static void* mapMemory( int shm_fd, std::size_t regionLength, bool lockPagesToRam );
};
//-----------------------CLASS----------------------//

//-------------------Static-funcs-------------------//
void mmapIPC::freeCallback(Napi::Env, void* memoryAddress, shm_metadata* hint)
{
    munmap(memoryAddress, hint->mmapLength);
    close(hint->shm_fd);
    shm_unlink(hint->shm_fileName.c_str());
    //free(hint);   //Seemingly, JS frees this automatically, so it causes double free error! UPDATE: Silly me, shm_hint defined on the stack!
}

int mmapIPC::createShmFile( const char *name, std::size_t memLenght)
{
    int shm_fd = shm_open(name, O_RDWR | O_CREAT | O_TRUNC, 00777);
    ftruncate(shm_fd, memLenght);
    return shm_fd;
}

void* mmapIPC::mapMemory( int shm_fd, std::size_t regionLength, bool lockPagesToRam )
{
    void* memoryAddress = mmap( (void*)0, regionLength, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    //fprintf(stderr, "memoryAddress: %p\n", memoryAddress);
    if( lockPagesToRam )
    {
        mlock2(memoryAddress, regionLength, MLOCK_ONFAULT);
    }

    return memoryAddress;
}
//-------------------Static-funcs-------------------//

//-----------------JS-Requirements-----------------//
Napi::Object mmapIPC::Init(Napi::Env env, Napi::Object exports)
{
    // This method is used to hook the accessor and method callbacks
    Napi::Function functionList = DefineClass(env, "mmapIPC",
    {
        InstanceMethod<&mmapIPC::buffer>("buffer", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
        InstanceMethod<&mmapIPC::acquireReadLock>("acquireReadLock", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
        InstanceMethod<&mmapIPC::acquireWriteLock>("acquireWriteLock", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
        InstanceMethod<&mmapIPC::removeLock>("removeLock", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
        StaticMethod<&mmapIPC::CreateNewItem>("CreateNewItem", static_cast<napi_property_attributes>(napi_writable | napi_configurable))
    });

    Napi::FunctionReference* constructor = new Napi::FunctionReference();

    *constructor = Napi::Persistent(functionList);
    exports.Set("mmapIPC", functionList);
    env.SetInstanceData<Napi::FunctionReference>(constructor);

    return exports;
}

Napi::Value mmapIPC::CreateNewItem(const Napi::CallbackInfo& info)
{
  Napi::FunctionReference* constructor = info.Env().GetInstanceData<Napi::FunctionReference>();
  return constructor->New({ info[0], info[1], info[2] });   //TODO: check: wrong; CreateNewItem is used only if we want to create a new JS object inside c++
}

Napi::Object Init (Napi::Env env, Napi::Object exports)
{
    mmapIPC::Init(env, exports);
    return exports;
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)
//-----------------JS-Requirements-----------------//

//===========================================================================================================================================================================================
mmapIPC::mmapIPC(const Napi::CallbackInfo& info) : Napi::ObjectWrap<mmapIPC>(info), shm_hint()
{
    Napi::Env env = info.Env();
    this->shm_hint.shm_fileName = "/" + std::string(info[0].ToString());
    this->shm_hint.mmapLength = (std::size_t)info[1].As<Napi::Number>().Int64Value();
    bool lockMemory = info[2].As<Napi::Boolean>().Value();

    this->shm_hint.shm_fd = createShmFile(this->shm_hint.shm_fileName.c_str(), this->shm_hint.mmapLength);
    void* externalData = mapMemory(this->shm_hint.shm_fd, this->shm_hint.mmapLength, lockMemory);

    this->memoryBuffer = Napi::Persistent(Napi::Buffer<uint8_t>::New(
                            env,
                            static_cast<uint8_t*>(externalData),
                            this->shm_hint.mmapLength,
                            freeCallback,
                            &this->shm_hint ));
    
    /*this->memoryBuffer = Napi::Buffer<void>::New(
                        env,
                        externalData,
                        this->shm_hint.mmapLength,
                        freeCallback,
                        &this->shm_hint);*/
}

Napi::Value mmapIPC::buffer(const Napi::CallbackInfo& info)
{
    return this->memoryBuffer.Value();
}

Napi::Value mmapIPC::acquireReadLock(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    
    if( flock(this->shm_hint.shm_fd, LOCK_SH) == 0 )
    { return Napi::Boolean::New(env, true); }
    else
    { return Napi::Boolean::New(env, false); }
}

Napi::Value mmapIPC::acquireWriteLock(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    
    if( flock(this->shm_hint.shm_fd, LOCK_EX) == 0 )
    { return Napi::Boolean::New(env, true); }
    else
    { return Napi::Boolean::New(env, false); }
}

Napi::Value mmapIPC::removeLock(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    
    if( flock(this->shm_hint.shm_fd, LOCK_UN) == 0 )
    { return Napi::Boolean::New(env, true); }
    else
    { return Napi::Boolean::New(env, false); }
}