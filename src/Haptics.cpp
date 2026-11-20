// Haptics.cpp

#include "Haptics.h"

#include "Platform.h"

#if defined(GAME_WEB)
#include <emscripten/html5.h>   // emscripten_vibrate: the browser's vibration
#elif defined(GAME_ANDROID)
#include <android/api-level.h>  // android_get_device_api_level
#include <android_native_app_glue.h>
#include <jni.h>                // JNI: how C++ talks to Java
// raylib keeps the "android_app" (our window on Android) and hands it out here.
extern "C" struct android_app* GetAndroidApp(void);
#endif

namespace haptics {

namespace {

bool gEnabled = true;

// How long each kind of buzz lasts, in milliseconds.
int Milliseconds(Buzz buzz) {
    switch (buzz) {
        case Buzz::Tick:     return 25;
        case Buzz::Hit:      return 90;
        case Buzz::GameOver: return 300;
        case Buzz::BossDown: return 450;
    }
    return 50;
}

#if defined(GAME_ANDROID)
// Android's vibration motor is reached through Java: Vibrator, a Java object. From
// C++ we use JNI ("Java Native Interface"): find the Java class, find the method
// by name and signature, then call it. The signature strings describe the
// parameters: "(J)V" = takes a long (J), returns nothing (V).
//
// The game runs on its own thread, which Java does not know yet. AttachCurrentThread
// introduces it once; after that GetEnv finds it.
JNIEnv* Env() {
    JavaVM* vm = GetAndroidApp()->activity->vm;
    JNIEnv* env = nullptr;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        if (vm->AttachCurrentThread(&env, nullptr) != JNI_OK) return nullptr;
    }
    return env;
}

// The phone's Vibrator, found once and then kept ("global reference"); nullptr
// when there is none (no motor, or something went wrong).
jobject Vibrator(JNIEnv* env) {
    static bool searched = false;
    static jobject vibrator = nullptr;
    if (searched) return vibrator;
    searched = true;

    // vibrator = activity.getSystemService(Context.VIBRATOR_SERVICE)
    jobject activity = GetAndroidApp()->activity->clazz;
    jclass contextClass = env->FindClass("android/content/Context");
    jfieldID serviceField = env->GetStaticFieldID(contextClass, "VIBRATOR_SERVICE", "Ljava/lang/String;");
    jobject serviceName = env->GetStaticObjectField(contextClass, serviceField);
    jclass activityClass = env->GetObjectClass(activity);
    jmethodID getSystemService =
        env->GetMethodID(activityClass, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
    jobject local = env->CallObjectMethod(activity, getSystemService, serviceName);

    if (local != nullptr) {
        // Does this device have a motor at all? (Many tablets do not.)
        jclass vibratorClass = env->GetObjectClass(local);
        jmethodID hasVibrator = env->GetMethodID(vibratorClass, "hasVibrator", "()Z");
        if (env->CallBooleanMethod(local, hasVibrator)) vibrator = env->NewGlobalRef(local);
        env->DeleteLocalRef(vibratorClass);
        env->DeleteLocalRef(local);
    }
    env->DeleteLocalRef(activityClass);
    env->DeleteLocalRef(serviceName);
    env->DeleteLocalRef(contextClass);
    if (env->ExceptionCheck()) env->ExceptionClear();   // a Java error: just no vibration
    return vibrator;
}

void AndroidVibrate(int milliseconds) {
    JNIEnv* env = Env();
    if (env == nullptr) return;
    jobject vibrator = Vibrator(env);
    if (vibrator == nullptr) return;

    jclass vibratorClass = env->GetObjectClass(vibrator);
    if (android_get_device_api_level() >= 26) {
        // Android 8 and newer: vibrator.vibrate(VibrationEffect.createOneShot(ms, DEFAULT_AMPLITUDE))
        jclass effectClass = env->FindClass("android/os/VibrationEffect");
        jmethodID createOneShot = env->GetStaticMethodID(effectClass, "createOneShot", "(JI)Landroid/os/VibrationEffect;");
        constexpr jint kDefaultAmplitude = -1;
        jobject effect = env->CallStaticObjectMethod(effectClass, createOneShot, static_cast<jlong>(milliseconds),
                                                     kDefaultAmplitude);
        jmethodID vibrate = env->GetMethodID(vibratorClass, "vibrate", "(Landroid/os/VibrationEffect;)V");
        env->CallVoidMethod(vibrator, vibrate, effect);
        env->DeleteLocalRef(effect);
        env->DeleteLocalRef(effectClass);
    } else {
        // Android 7: the older vibrator.vibrate(ms)
        jmethodID vibrate = env->GetMethodID(vibratorClass, "vibrate", "(J)V");
        env->CallVoidMethod(vibrator, vibrate, static_cast<jlong>(milliseconds));
    }
    env->DeleteLocalRef(vibratorClass);
    if (env->ExceptionCheck()) env->ExceptionClear();
}
#endif

}  // namespace

void SetEnabled(bool enabled) { gEnabled = enabled; }

void Vibrate(Buzz buzz) {
    if (!gEnabled) return;
    const int milliseconds = Milliseconds(buzz);
#if defined(GAME_WEB)
    emscripten_vibrate(milliseconds);
#elif defined(GAME_ANDROID)
    AndroidVibrate(milliseconds);
#else
    (void)milliseconds;   // a desktop computer cannot vibrate
#endif
}

}  // namespace haptics
