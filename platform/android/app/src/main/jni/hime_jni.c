/*
 * HIME Android JNI Bridge
 *
 * JNI interface between hime-core C library and Java/Kotlin code.
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

#include <stdlib.h>
#include <string.h>

#include <jni.h>

#include "hime-core.h"

/* Global context - one per app instance */
static HimeContext *g_context = NULL;

/* Java callback reference */
static JavaVM *g_jvm = NULL;
static jobject g_engine_ref = NULL;

/*
 * Helper: Convert Java string to C string
 */
static char *jstring_to_cstr (JNIEnv *env, jstring jstr) {
    if (jstr == NULL)
        return NULL;
    const char *str = (*env)->GetStringUTFChars (env, jstr, NULL);
    char *result = strdup (str);
    (*env)->ReleaseStringUTFChars (env, jstr, str);
    return result;
}

/*
 * Helper: Convert C string to Java string
 */
static jstring cstr_to_jstring (JNIEnv *env, const char *str) {
    if (str == NULL)
        return NULL;
    return (*env)->NewStringUTF (env, str);
}

/*
 * Feedback callback - called from hime-core
 */
static void feedback_callback (HimeFeedbackType type, void *userData) {
    if (g_jvm == NULL || g_engine_ref == NULL)
        return;

    JNIEnv *env;
    int attached = 0;

    if ((*g_jvm)->GetEnv (g_jvm, (void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        if ((*g_jvm)->AttachCurrentThread (g_jvm, &env, NULL) != JNI_OK) {
            return;
        }
        attached = 1;
    }

    jclass cls = (*env)->GetObjectClass (env, g_engine_ref);
    jmethodID method = (*env)->GetMethodID (env, cls, "onNativeFeedback", "(I)V");
    if (method != NULL) {
        (*env)->CallVoidMethod (env, g_engine_ref, method, (jint) type);
    }

    if (attached) {
        (*g_jvm)->DetachCurrentThread (g_jvm);
    }
}

/*
 * Class:     org_hime_android_core_HimeEngine
 * Method:    nativeInit
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL
Java_org_hime_android_core_HimeEngine_nativeInit (JNIEnv *env, jobject thiz, jstring dataPath) {
    /* Store JVM reference for callbacks */
    (*env)->GetJavaVM (env, &g_jvm);

    /* Store engine reference for callbacks */
    if (g_engine_ref != NULL) {
        (*env)->DeleteGlobalRef (env, g_engine_ref);
    }
    g_engine_ref = (*env)->NewGlobalRef (env, thiz);

    if (g_context != NULL) {
        hime_context_free (g_context);
        g_context = NULL;
    }

    char *path = jstring_to_cstr (env, dataPath);
    int result = hime_init (path);
    free (path);

    if (result != 0) {
        return JNI_FALSE;
    }

    g_context = hime_context_new ();
    if (g_context == NULL) {
        return JNI_FALSE;
    }

    /* Set up feedback callback */
    hime_set_feedback_callback (g_context, feedback_callback, NULL);

    return JNI_TRUE;
}

/*
 * Class:     org_hime_android_core_HimeEngine
 * Method:    nativeDestroy
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_org_hime_android_core_HimeEngine_nativeDestroy (JNIEnv *env, jobject thiz) {
    if (g_context != NULL) {
        hime_context_free (g_context);
        g_context = NULL;
    }
    hime_cleanup ();

    if (g_engine_ref != NULL) {
        (*env)->DeleteGlobalRef (env, g_engine_ref);
        g_engine_ref = NULL;
    }
}

/*
 * Class:     org_hime_android_core_HimeEngine
 * Method:    nativeProcessKey
 * Signature: (CI)I
 */
JNIEXPORT jint JNICALL
Java_org_hime_android_core_HimeEngine_nativeProcessKey (JNIEnv *env, jobject thiz, jchar keyChar, jint modifiers) {
    if (g_context == NULL)
        return HIME_KEY_IGNORED;
    return hime_process_key (g_context, 0, (uint32_t) keyChar, (uint32_t) modifiers);
}

/*
 * Class:     org_hime_android_core_HimeEngine
 * Method:    nativeGetPreedit
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_org_hime_android_core_HimeEngine_nativeGetPreedit (JNIEnv *env, jobject thiz) {
    if (g_context == NULL)
        return NULL;

    char buffer[HIME_MAX_PREEDIT];
    int len = hime_get_preedit (g_context, buffer, sizeof (buffer));
    if (len <= 0)
        return NULL;

    return cstr_to_jstring (env, buffer);
}

/*
 * Class:     org_hime_android_core_HimeEngine
 * Method:    nativeGetCommit
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_org_hime_android_core_HimeEngine_nativeGetCommit (JNIEnv *env, jobject thiz) {
    if (g_context == NULL)
        return NULL;

    char buffer[HIME_MAX_PREEDIT];
    int len = hime_get_commit (g_context, buffer, sizeof (buffer));
    if (len <= 0)
        return NULL;

    return cstr_to_jstring (env, buffer);
}

/*
 * Class:     org_hime_android_core_HimeEngine
 * Method:    nativeGetCandidates
 * Signature: (I)[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL
Java_org_hime_android_core_HimeEngine_nativeGetCandidates (JNIEnv *env, jobject thiz, jint page) {
    if (g_context == NULL)
        return NULL;

    int total = hime_get_candidate_count (g_context);
    if (total == 0)
        return NULL;

    int per_page = hime_get_candidates_per_page (g_context);
    int start = page * per_page;
    int count = total - start;
    if (count > per_page)
        count = per_page;
    if (count <= 0)
        return NULL;

    jclass stringClass = (*env)->FindClass (env, "java/lang/String");
    jobjectArray result = (*env)->NewObjectArray (env, count, stringClass, NULL);

    char buffer[HIME_MAX_CANDIDATE_LEN];
    for (int i = 0; i < count; i++) {
        int len = hime_get_candidate (g_context, start + i, buffer, sizeof (buffer));
        if (len > 0) {
            jstring str = cstr_to_jstring (env, buffer);
            (*env)->SetObjectArrayElement (env, result, i, str);
            (*env)->DeleteLocalRef (env, str);
        }
    }

    return result;
}

/*
 * Class:     org_hime_android_core_HimeEngine
 * Method:    nativeGetCandidateCount
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_org_hime_android_core_HimeEngine_nativeGetCandidateCount (JNIEnv *env, jobject thiz) {
    if (g_context == NULL)
        return 0;
    return hime_get_candidate_count (g_context);
}

/*
 * Class:     org_hime_android_core_HimeEngine
 * Method:    nativeSelectCandidate
 * Signature: (I)Z
 */
JNIEXPORT jboolean JNICALL
Java_org_hime_android_core_HimeEngine_nativeSelectCandidate (JNIEnv *env, jobject thiz, jint index) {
    if (g_context == NULL)
        return JNI_FALSE;
    return hime_select_candidate (g_context, index) == HIME_KEY_COMMIT ? JNI_TRUE : JNI_FALSE;
}

/*
 * Class:     org_hime_android_core_HimeEngine
 * Method:    nativeClearPreedit
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_org_hime_android_core_HimeEngine_nativeClearPreedit (JNIEnv *env, jobject thiz) {
    if (g_context != NULL) {
        hime_context_reset (g_context);
    }
}

/*
 * Class:     org_hime_android_core_HimeEngine
 * Method:    nativeReset
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_org_hime_android_core_HimeEngine_nativeReset (JNIEnv *env, jobject thiz) {
    if (g_context != NULL) {
        hime_context_reset (g_context);
    }
}

/*
 * Class:     org_hime_android_core_HimeEngine
 * Method:    nativeSetInputMode
 * Signature: (I)V
 */
JNIEXPORT void JNICALL
Java_org_hime_android_core_HimeEngine_nativeSetInputMode (JNIEnv *env, jobject thiz, jint mode) {
    if (g_context != NULL) {
        hime_set_chinese_mode (g_context, mode == 0); /* 0 = Chinese */
    }
}

/*
 * Class:     org_hime_android_core_HimeEngine
 * Method:    nativeGetInputMode
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_org_hime_android_core_HimeEngine_nativeGetInputMode (JNIEnv *env, jobject thiz) {
    if (g_context == NULL)
        return 0;
    return hime_is_chinese_mode (g_context) ? 0 : 1; /* 0 = Chinese, 1 = English */
}

/* ========== Character Set ========== */

JNIEXPORT jint JNICALL
Java_org_hime_android_core_HimeEngine_nativeGetCharset (JNIEnv *env, jobject thiz) {
    if (g_context == NULL)
        return 0;
    return (jint) hime_get_charset (g_context);
}

JNIEXPORT void JNICALL
Java_org_hime_android_core_HimeEngine_nativeSetCharset (JNIEnv *env, jobject thiz, jint charset) {
    if (g_context != NULL) {
        hime_set_charset (g_context, (HimeCharset) charset);
    }
}

JNIEXPORT jint JNICALL
Java_org_hime_android_core_HimeEngine_nativeToggleCharset (JNIEnv *env, jobject thiz) {
    if (g_context == NULL)
        return 0;
    return (jint) hime_toggle_charset (g_context);
}

/* ========== Smart Punctuation ========== */

JNIEXPORT jboolean JNICALL
Java_org_hime_android_core_HimeEngine_nativeGetSmartPunctuation (JNIEnv *env, jobject thiz) {
    if (g_context == NULL)
        return JNI_FALSE;
    return hime_get_smart_punctuation (g_context) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_org_hime_android_core_HimeEngine_nativeSetSmartPunctuation (JNIEnv *env, jobject thiz, jboolean enabled) {
    if (g_context != NULL) {
        hime_set_smart_punctuation (g_context, enabled == JNI_TRUE);
    }
}

JNIEXPORT jstring JNICALL
Java_org_hime_android_core_HimeEngine_nativeConvertPunctuation (JNIEnv *env, jobject thiz, jchar ascii) {
    if (g_context == NULL)
        return NULL;

    char buffer[16];
    int len = hime_convert_punctuation (g_context, (char) ascii, buffer, sizeof (buffer));
    if (len <= 0)
        return NULL;

    return cstr_to_jstring (env, buffer);
}

JNIEXPORT void JNICALL
Java_org_hime_android_core_HimeEngine_nativeResetPunctuationState (JNIEnv *env, jobject thiz) {
    if (g_context != NULL) {
        hime_reset_punctuation_state (g_context);
    }
}

/* ========== Pinyin Annotation ========== */

JNIEXPORT jboolean JNICALL
Java_org_hime_android_core_HimeEngine_nativeGetPinyinAnnotation (JNIEnv *env, jobject thiz) {
    if (g_context == NULL)
        return JNI_FALSE;
    return hime_get_pinyin_annotation (g_context) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_org_hime_android_core_HimeEngine_nativeSetPinyinAnnotation (JNIEnv *env, jobject thiz, jboolean enabled) {
    if (g_context != NULL) {
        hime_set_pinyin_annotation (g_context, enabled == JNI_TRUE);
    }
}

/* ========== Candidate Style ========== */

JNIEXPORT jint JNICALL
Java_org_hime_android_core_HimeEngine_nativeGetCandidateStyle (JNIEnv *env, jobject thiz) {
    if (g_context == NULL)
        return 0;
    return (jint) hime_get_candidate_style (g_context);
}

JNIEXPORT void JNICALL
Java_org_hime_android_core_HimeEngine_nativeSetCandidateStyle (JNIEnv *env, jobject thiz, jint style) {
    if (g_context != NULL) {
        hime_set_candidate_style (g_context, (HimeCandidateStyle) style);
    }
}

/* ========== Color Scheme ========== */

JNIEXPORT jint JNICALL
Java_org_hime_android_core_HimeEngine_nativeGetColorScheme (JNIEnv *env, jobject thiz) {
    if (g_context == NULL)
        return 0;
    return (jint) hime_get_color_scheme (g_context);
}

JNIEXPORT void JNICALL
Java_org_hime_android_core_HimeEngine_nativeSetColorScheme (JNIEnv *env, jobject thiz, jint scheme) {
    if (g_context != NULL) {
        hime_set_color_scheme (g_context, (HimeColorScheme) scheme);
    }
}

JNIEXPORT void JNICALL
Java_org_hime_android_core_HimeEngine_nativeSetSystemDarkMode (JNIEnv *env, jobject thiz, jboolean isDark) {
    if (g_context != NULL) {
        hime_set_system_dark_mode (g_context, isDark == JNI_TRUE);
    }
}

/* ========== Feedback Settings ========== */

JNIEXPORT jboolean JNICALL
Java_org_hime_android_core_HimeEngine_nativeGetSoundEnabled (JNIEnv *env, jobject thiz) {
    if (g_context == NULL)
        return JNI_FALSE;
    return hime_get_sound_enabled (g_context) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_org_hime_android_core_HimeEngine_nativeSetSoundEnabled (JNIEnv *env, jobject thiz, jboolean enabled) {
    if (g_context != NULL) {
        hime_set_sound_enabled (g_context, enabled == JNI_TRUE);
    }
}

JNIEXPORT jboolean JNICALL
Java_org_hime_android_core_HimeEngine_nativeGetVibrationEnabled (JNIEnv *env, jobject thiz) {
    if (g_context == NULL)
        return JNI_FALSE;
    return hime_get_vibration_enabled (g_context) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_org_hime_android_core_HimeEngine_nativeSetVibrationEnabled (JNIEnv *env, jobject thiz, jboolean enabled) {
    if (g_context != NULL) {
        hime_set_vibration_enabled (g_context, enabled == JNI_TRUE);
    }
}

JNIEXPORT jint JNICALL
Java_org_hime_android_core_HimeEngine_nativeGetVibrationDuration (JNIEnv *env, jobject thiz) {
    if (g_context == NULL)
        return 20;
    return hime_get_vibration_duration (g_context);
}

JNIEXPORT void JNICALL
Java_org_hime_android_core_HimeEngine_nativeSetVibrationDuration (JNIEnv *env, jobject thiz, jint durationMs) {
    if (g_context != NULL) {
        hime_set_vibration_duration (g_context, durationMs);
    }
}

/* ========== Keyboard Layout ========== */

JNIEXPORT jint JNICALL
Java_org_hime_android_core_HimeEngine_nativeGetKeyboardLayout (JNIEnv *env, jobject thiz) {
    if (g_context == NULL)
        return 0;
    return (jint) hime_get_keyboard_layout (g_context);
}

JNIEXPORT jint JNICALL
Java_org_hime_android_core_HimeEngine_nativeSetKeyboardLayout (JNIEnv *env, jobject thiz, jint layout) {
    if (g_context == NULL)
        return -1;
    return hime_set_keyboard_layout (g_context, (HimeKeyboardLayout) layout);
}

JNIEXPORT jint JNICALL
Java_org_hime_android_core_HimeEngine_nativeSetKeyboardLayoutByName (JNIEnv *env, jobject thiz, jstring layoutName) {
    if (g_context == NULL || layoutName == NULL)
        return -1;

    char *name = jstring_to_cstr (env, layoutName);
    int result = hime_set_keyboard_layout_by_name (g_context, name);
    free (name);

    return result;
}
