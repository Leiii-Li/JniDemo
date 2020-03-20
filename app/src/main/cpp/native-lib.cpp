#include <jni.h>
#include <string>
#include <pthread.h>
#include <android/log.h>
#include <inttypes.h>

// Android log function wrappers
static const char *kTAG = "[nelson] - jni";
#define LOGI(...) \
  ((void)__android_log_print(ANDROID_LOG_INFO, kTAG, __VA_ARGS__))
#define LOGW(...) \
  ((void)__android_log_print(ANDROID_LOG_WARN, kTAG, __VA_ARGS__))
#define LOGE(...) \
  ((void)__android_log_print(ANDROID_LOG_ERROR, kTAG, __VA_ARGS__))

// processing callback to handler class
typedef struct tick_context {
  JavaVM *javaVM;
  jclass jniHelperClz;
  jobject jniHelperObj;
  jclass mainActivityClz;
  jobject mainActivityObj;
  pthread_mutex_t lock;
  int done;
} TickContext;
TickContext g_ctx;


void queryRuntimeInfo(JNIEnv *env, jobject pJobject);
void *UpdateTickets(void *);
void sendJavaMsg(JNIEnv *env, jobject instance, jmethodID methodId, const char msg[40]);

/**
 * 用于初始化
 * @param vm JVM实例
 * @param reserved
 * @return
 * @Note 这里分配的所有资源都不会被应用程序释放我们依靠系统来释放所有的全局引用，当它消失的时候;
 *       根本不会调用配对函数JNI_OnUnload()。
 */
jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    //jni
    JNIEnv *env;

    // 申请内存空间
    memset(&g_ctx, 0, sizeof(g_ctx));

    g_ctx.javaVM = vm;

    LOGI("Jni On Load");

    // 获取 JniNativeInterface
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR; // JNI version not supported.
    }

    // 获取 JniHandler 类
    jclass clz = env->FindClass("com/nelson/jnicallback/JniHandler");

    g_ctx.jniHelperClz = static_cast<jclass>(env->NewGlobalRef(clz));

    jmethodID jniHandlerConstruct = env->GetMethodID(clz, "<init>", "()V");

    jobject jniHandlerInstance = env->NewObject(clz, jniHandlerConstruct);

    g_ctx.jniHelperObj = env->NewGlobalRef(jniHandlerInstance);

    queryRuntimeInfo(env, g_ctx.jniHelperObj);

    g_ctx.done = 0;

    g_ctx.mainActivityObj = NULL;

    return JNI_VERSION_1_6;
}

JNIEXPORT void queryRuntimeInfo(JNIEnv *env, jobject obj) {
    jmethodID versionFunctionMethod =
            env->GetStaticMethodID(g_ctx.jniHelperClz, "getBuildVersion", "()Ljava/lang/String;");

    if (!versionFunctionMethod) {
        LOGE("无法获取JniHandler 中的 getBuildVersion 方法 @line %d", __LINE__);
        return;
    }

    jstring buildVersion = static_cast<jstring>(env->CallStaticObjectMethod(g_ctx.jniHelperClz,
                                                                            versionFunctionMethod));

    const char *version = env->GetStringUTFChars(buildVersion, NULL);

    LOGI("Android Version - %s ", version);

    env->ReleaseStringUTFChars(buildVersion, version);

    env->DeleteLocalRef(buildVersion);

    jmethodID getRuntimeMemorySizeMethod =
            env->GetMethodID(g_ctx.jniHelperClz, "getRuntimeMemorySize", "()J");

    jlong memorySize = env->CallLongMethod(obj, getRuntimeMemorySizeMethod);

    LOGI("Runtime free memory size: %"
                 PRId64, memorySize);
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_nelson_jnicallback_MainActivity_startTickets(
        JNIEnv *env,
        jobject obj) {

    pthread_t threadInfo;
    pthread_attr_t threadAttr;

    pthread_attr_init(&threadAttr);
    pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_JOINABLE);

    pthread_mutex_init(&g_ctx.lock, NULL);

    jclass clz = env->GetObjectClass(obj);

    g_ctx.mainActivityClz = static_cast<jclass>(env->NewGlobalRef(clz));
    g_ctx.mainActivityObj = env->NewGlobalRef(obj);

    int result = pthread_create(&threadInfo, &threadAttr, UpdateTickets, &g_ctx);

    assert(result == 0);

    pthread_attr_destroy(&threadAttr);

    std::string hello = "Hello from C++";

    LOGI("Thread Create Success %d", result);

    return env->NewStringUTF(hello.c_str());
}

void *UpdateTickets(void *context) {
    TickContext *tickContext = (TickContext *) context;
    JavaVM *javaVm = tickContext->javaVM;
    JNIEnv *env;

    jint res = javaVm->GetEnv((void **) &env, JNI_VERSION_1_6);

    if (res != JNI_OK) {
        res = javaVm->AttachCurrentThread(&env, NULL);
        if (JNI_OK != res) {
            LOGE("Failed to AttachCurrentThread, ErrorCode = %d", res);
            return NULL;
        }
    }

    jmethodID updateStatusMethodId =
            env->GetMethodID(tickContext->jniHelperClz, "updateStatus", "(Ljava/lang/String;)V");

    sendJavaMsg(env,
                tickContext->jniHelperObj,
                updateStatusMethodId,
                "Ticker Thread Status : initializing ...");

    jmethodID timberMethodId = env->GetMethodID(tickContext->mainActivityClz, "updateTimer", "()V");

    struct timeval beginTime, currentTime, usedTime, leftTime;

    const struct timeval kOneSecond = {
            (__kernel_time_t) 1,
            (__kernel_suseconds_t) 0
    };

    sendJavaMsg(env,
                tickContext->jniHelperObj,
                updateStatusMethodId,
                "TickerThread Status : Start ticking");

    while (1) {
        gettimeofday(&beginTime, NULL);
        pthread_mutex_lock(&tickContext->lock);
        int done = tickContext->done;

        if (tickContext->done) {
            tickContext->done = 0;
        }
        pthread_mutex_unlock(&tickContext->lock);

        if (done) {
            break;
        }

        env->CallVoidMethod(tickContext->mainActivityObj, timberMethodId);

        gettimeofday(&currentTime, NULL);
        // 获取 beginTime与currentTime的时间间隔
        timersub(&currentTime, &beginTime, &usedTime);
        timersub(&kOneSecond, &usedTime, &leftTime);
        struct timespec sleepTime;
        sleepTime.tv_sec = leftTime.tv_sec;
        sleepTime.tv_nsec = leftTime.tv_usec * 1000;

        if (sleepTime.tv_sec <= 1) {
            nanosleep(&sleepTime, NULL);
        } else {
            sendJavaMsg(env, tickContext->jniHelperObj, updateStatusMethodId,
                        "TickerThread error: processing too long!");
        }
    }
    sendJavaMsg(env,
                tickContext->jniHelperObj,
                updateStatusMethodId,
                "TickerThread status: ticking stopped");
    javaVm->DetachCurrentThread();
    return NULL;
}

void sendJavaMsg(JNIEnv *env, jobject instance, jmethodID methodId, const char *msg) {

    jstring javaMsg = env->NewStringUTF(msg);

    env->CallVoidMethod(instance, methodId, javaMsg);

    env->DeleteLocalRef(javaMsg);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_nelson_jnicallback_MainActivity_stopTickets(JNIEnv *env, jobject thiz) {
    pthread_mutex_lock(&g_ctx.lock);
    g_ctx.done = 1;
    pthread_mutex_unlock(&g_ctx.lock);

    struct timespec sleepTime;
    memset(&sleepTime, 0, sizeof(sleepTime));
    sleepTime.tv_nsec = 100000000;
    while (g_ctx.done) {
        nanosleep(&sleepTime, NULL);
    }

    // release object we allocated from StartTicks() function
    env->DeleteGlobalRef(g_ctx.mainActivityClz);
    env->DeleteGlobalRef(g_ctx.mainActivityObj);
    g_ctx.mainActivityObj = NULL;
    g_ctx.mainActivityClz = NULL;

    pthread_mutex_destroy(&g_ctx.lock);
}