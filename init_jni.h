/**
 * Description:
 * Author:created by bob on 17-5-27.
 */
//

#ifndef DVFLYING2_INIT_JNI_H
#define DVFLYING2_INIT_JNI_H
#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#define EXCEPTION_CLASS  "com/exception/NativeException"
#define throw_e(e, str) \
    {\
      jclass clazz = (*e)->FindClass(e, EXCEPTION_CLASS);\
      if (clazz != NULL){\
        (*e)->ThrowNew(e, clazz, str);\
    }\
  }
enum RECORD_STATE
{
    REC_STATE_START = 1,
    REC_STATE_END = 2,
};
enum RECORD_ERROR
{
    REC_ERR_PATH = -1,
    REC_ERR_ = -2,
};
JNIEnv *attach_current_thread(JavaVM *vm);
void detach_current_thread(JavaVM *vm);
#endif //INIT_JNI_H
