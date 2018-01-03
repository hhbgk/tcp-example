/**
 * Description:
 * Author:created by bob on 17-5-27.
 */

#include <jni.h>
#include <stddef.h>
#include <assert.h>
#include "debug.h"
#include "tcp_server_jni.h"
#include "tcp_client_jni.h"

#define tag "JNI"
JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
    JNIEnv *env = NULL;
    if ((*vm)->GetEnv(vm, (void **) &env, JNI_VERSION_1_6) != JNI_OK)
    {
        loge(tag, "GetEnv failed!");
        return JNI_ERR;
    }

    register_tcp_client(env);
    return JNI_VERSION_1_6;
}

JNIEnv *attach_current_thread(JavaVM *vm)
{
    assert(vm != NULL);
    JNIEnv *env = NULL;
    if ((*vm)->GetEnv(vm, (void **) &env, JNI_VERSION_1_6) < 0)
    {
        if ((*vm)->AttachCurrentThread(vm, &env, NULL) < 0)
        {
            return NULL;
        }
    }
    assert(env != NULL);
    return env;
}
void detach_current_thread(JavaVM *vm)
{
    assert(vm != NULL);
    (*vm)->DetachCurrentThread(vm);
}
