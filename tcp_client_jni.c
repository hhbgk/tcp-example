/**
 * Description:
 * Author:created by bob on 17-12-27.
 */
//

#include "tcp_client_jni.h"
#include <assert.h>
#include "tcp_server_jni.h"
#include "init_jni.h"

#define tag "TCP_SERVER_JNI"
#define CLASS_TCP_SERVER "com/player/protocol/TCPRTStreamImpl"
static JavaVM *gJVM = NULL;
static jobject gObj = NULL;
static tcp_cli_t g_tcp_cli;
static jmethodID on_frame_cb_id;
static jmethodID on_error_cb_id;
static jmethodID on_state_changed_cb_id;

static void on_error(const int code, const char *msg)
{
    JNIEnv *env = attach_current_thread(gJVM);
    assert(env != NULL);
    jstring str = (*env)->NewStringUTF(env, msg);
    (*env)->CallVoidMethod(env, gObj, on_error_cb_id, code, str);
    (*env)->DeleteLocalRef(env, str);
    detach_current_thread(gJVM);
}
static void on_frame_received(int type, uint8_t *buf, uint32_t size, uint32_t sequence, uint32_t timestamp)
{
    JNIEnv *env = attach_current_thread(gJVM);
    assert(env != NULL);
    switch (type)
    {
        case TYPE_RTS_PCM:
        case TYPE_RTS_H264:
        case TYPE_RTS_JPEG:
        {
            jbyteArray jArray = NULL;
            jArray = (*env)->NewByteArray(env, (jsize) size);
            (*env)->SetByteArrayRegion(env, jArray, 0, size, (jbyte *) (buf));
            //logi ("on_video_frame ....6.....packetHdr->sequence=%d", packetHdr->sequence);
            (*env)->CallVoidMethod(env, gObj, on_frame_cb_id, (jint) type, jArray, sequence, timestamp);
            if (jArray != NULL) (*env)->DeleteLocalRef(env, jArray);
        }
            break;
        default:
            loge(tag, "Error type:%d", type);
            break;
    }
    detach_current_thread(gJVM);
}
static void on_state_changed(int status)
{
    //logi(tag, "%s:%d", __func__, status);
    JNIEnv *env = attach_current_thread(gJVM);
    assert(env != NULL);
    (*env)->CallVoidMethod (env, gObj, on_state_changed_cb_id, (jint)status);
    detach_current_thread(gJVM);
}
static jboolean jni_create(JNIEnv *env, jobject thiz, jint jport)
{
    tcp_cli_t *tcpServer = &g_tcp_cli;
    tcp_client_set_error_callback(tcpServer, on_error);
    tcp_client_set_state_changed_callback(tcpServer, on_state_changed);
    tcp_client_set_frame_received_callback(tcpServer, on_frame_received);
    return (jboolean) (tcp_client_create(tcpServer) != 0 ? JNI_FALSE : JNI_TRUE);
}
static jboolean jni_close(JNIEnv *env, jobject thiz)
{
    tcp_cli_t *tcpServer = &g_tcp_cli;
    return (jboolean) (tcp_client_close(tcpServer) != 0 ? JNI_FALSE : JNI_TRUE);
}
static jboolean jni_is_receiving(JNIEnv *env, jobject thiz)
{
    tcp_cli_t *tcpServer = &g_tcp_cli;
    return (jboolean) (tcp_client_is_receiving(tcpServer) == 0 ? JNI_TRUE : JNI_FALSE);
}
static jboolean jni_set_frame_rate (JNIEnv *env, jobject thiz, jint jFrameRate)
{
    logi(tag, "%s:%d", __func__, jFrameRate);
    tcp_cli_t *tcpServer = &g_tcp_cli;
    tcp_client_set_fps(tcpServer, (uint32_t) jFrameRate);
    return JNI_TRUE;
}
static jboolean jni_set_sample_rate(JNIEnv *env, jobject thiz, jint jSampleRate)
{
    logi(tag, "%s:%d", __func__, jSampleRate);
    tcp_cli_t *tcpServer = &g_tcp_cli;
    tcp_client_set_sample_rate(tcpServer, (uint32_t) jSampleRate);
    return JNI_TRUE;
}
static jboolean jni_set_resolution (JNIEnv *env, jobject thiz, jint w, jint h)
{
    logi(tag, "%s:w=%d, h=%d", __func__, w, h);
    tcp_cli_t *tcpServer = &g_tcp_cli;
    assert(tcpServer != NULL);
    tcp_client_set_resolution(tcpServer, w, h);
    return JNI_TRUE;
}
static jboolean jni_init(JNIEnv *env, jobject thiz)
{
    (*env)->GetJavaVM(env, &gJVM);
    gObj = (*env)->NewGlobalRef(env, thiz);
    jclass clazz = (*env)->GetObjectClass(env, thiz);
    if (!clazz)
    {
        throw_e(env, "Can not find class");
        return JNI_FALSE;
    }
    on_frame_cb_id = (*env)->GetMethodID(env, clazz, "onFrameReceived", "(I[BJJ)V");
    on_error_cb_id = (*env)->GetMethodID(env, clazz, "onError", "(ILjava/lang/String;)V");
    on_state_changed_cb_id = (*env)->GetMethodID(env, clazz, "onStateChanged", "(I)V");
    if (!on_frame_cb_id || !on_error_cb_id || !on_state_changed_cb_id)
        logw(tag, "The calling class does not implement all necessary interface methods");
    return JNI_TRUE;
}
static JNINativeMethod g_methods[] =
        {
                {"nativeInit", "()Z", (void *) jni_init},
                {"nativeCreateClient", "(I)Z", (void*) jni_create},
                {"nativeCloseClient", "()Z", (void*) jni_close},
                {"nativeSetFrameRate", "(I)Z", (void*) jni_set_frame_rate},
                {"nativeSetSampleRate", "(I)Z", (void*) jni_set_sample_rate},
                {"nativeSetResolution", "(II)Z", (void*) jni_set_resolution},
                {"nativeIsReceiving", "()Z", (void*) jni_is_receiving},
        };

int register_tcp_client(JNIEnv* env)
{
    jclass klass = (*env)->FindClass(env, CLASS_TCP_SERVER);
    if (klass == NULL)
    {
        return JNI_ERR;
    }
    (*env)->RegisterNatives(env, klass, g_methods, NELEM(g_methods));

    return JNI_VERSION_1_6;
}
