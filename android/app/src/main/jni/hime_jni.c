/*
 * HIME Android JNI Bridge
 *
 * JNI interface between hime-core C library and Java/Kotlin code.
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

#include <jni.h>
#include <string.h>
#include <stdlib.h>
#include "hime-core.h"

/* Global context - one per app instance */
static HimeContext *g_context = NULL;

/*
 * Helper: Convert Java string to C string
 */
static char *jstring_to_cstr(JNIEnv *env, jstring jstr) {
    if (jstr == NULL) return NULL;
    const char *str = (*env)->GetStringUTFChars(env, jstr, NULL);
    char *result = strdup(str);
    (*env)->ReleaseStringUTFChars(env, jstr, str);
    return result;
}

/*
 * Helper: Convert C string to Java string
 */
static jstring cstr_to_jstring(JNIEnv *env, const char *str) {
    if (str == NULL) return NULL;
    return (*env)->NewStringUTF(env, str);
}

/*
 * Class:     org_hime_android_core_HimeEngine
 * Method:    nativeInit
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL
Java_org_hime_android_core_HimeEngine_nativeInit(JNIEnv *env, jobject thiz, jstring dataPath) {
    if (g_context != NULL) {
        hime_context_destroy(g_context);
    }

    char *path = jstring_to_cstr(env, dataPath);
    g_context = hime_context_create(path);
    free(path);

    return g_context != NULL ? JNI_TRUE : JNI_FALSE;
}

/*
 * Class:     org_hime_android_core_HimeEngine
 * Method:    nativeDestroy
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_org_hime_android_core_HimeEngine_nativeDestroy(JNIEnv *env, jobject thiz) {
    if (g_context != NULL) {
        hime_context_destroy(g_context);
        g_context = NULL;
    }
}

/*
 * Class:     org_hime_android_core_HimeEngine
 * Method:    nativeProcessKey
 * Signature: (CI)I
 */
JNIEXPORT jint JNICALL
Java_org_hime_android_core_HimeEngine_nativeProcessKey(JNIEnv *env, jobject thiz,
                                                        jchar keyChar, jint modifiers) {
    if (g_context == NULL) return HIME_RESULT_IGNORED;
    return hime_process_key(g_context, (char)keyChar, modifiers);
}

/*
 * Class:     org_hime_android_core_HimeEngine
 * Method:    nativeGetPreedit
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_org_hime_android_core_HimeEngine_nativeGetPreedit(JNIEnv *env, jobject thiz) {
    if (g_context == NULL) return NULL;
    const char *preedit = hime_get_preedit(g_context);
    return cstr_to_jstring(env, preedit);
}

/*
 * Class:     org_hime_android_core_HimeEngine
 * Method:    nativeGetCommit
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_org_hime_android_core_HimeEngine_nativeGetCommit(JNIEnv *env, jobject thiz) {
    if (g_context == NULL) return NULL;
    const char *commit = hime_get_commit(g_context);
    return cstr_to_jstring(env, commit);
}

/*
 * Class:     org_hime_android_core_HimeEngine
 * Method:    nativeGetCandidates
 * Signature: (I)[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL
Java_org_hime_android_core_HimeEngine_nativeGetCandidates(JNIEnv *env, jobject thiz, jint page) {
    if (g_context == NULL) return NULL;

    int count = 0;
    const char **candidates = hime_get_candidates(g_context, page, &count);

    if (candidates == NULL || count == 0) {
        return NULL;
    }

    jclass stringClass = (*env)->FindClass(env, "java/lang/String");
    jobjectArray result = (*env)->NewObjectArray(env, count, stringClass, NULL);

    for (int i = 0; i < count; i++) {
        jstring str = cstr_to_jstring(env, candidates[i]);
        (*env)->SetObjectArrayElement(env, result, i, str);
        (*env)->DeleteLocalRef(env, str);
    }

    return result;
}

/*
 * Class:     org_hime_android_core_HimeEngine
 * Method:    nativeGetCandidateCount
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_org_hime_android_core_HimeEngine_nativeGetCandidateCount(JNIEnv *env, jobject thiz) {
    if (g_context == NULL) return 0;
    return hime_get_candidate_count(g_context);
}

/*
 * Class:     org_hime_android_core_HimeEngine
 * Method:    nativeSelectCandidate
 * Signature: (I)Z
 */
JNIEXPORT jboolean JNICALL
Java_org_hime_android_core_HimeEngine_nativeSelectCandidate(JNIEnv *env, jobject thiz, jint index) {
    if (g_context == NULL) return JNI_FALSE;
    return hime_select_candidate(g_context, index) ? JNI_TRUE : JNI_FALSE;
}

/*
 * Class:     org_hime_android_core_HimeEngine
 * Method:    nativeClearPreedit
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_org_hime_android_core_HimeEngine_nativeClearPreedit(JNIEnv *env, jobject thiz) {
    if (g_context != NULL) {
        hime_clear_preedit(g_context);
    }
}

/*
 * Class:     org_hime_android_core_HimeEngine
 * Method:    nativeReset
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_org_hime_android_core_HimeEngine_nativeReset(JNIEnv *env, jobject thiz) {
    if (g_context != NULL) {
        hime_reset(g_context);
    }
}

/*
 * Class:     org_hime_android_core_HimeEngine
 * Method:    nativeSetInputMode
 * Signature: (I)V
 */
JNIEXPORT void JNICALL
Java_org_hime_android_core_HimeEngine_nativeSetInputMode(JNIEnv *env, jobject thiz, jint mode) {
    if (g_context != NULL) {
        hime_set_input_mode(g_context, mode);
    }
}

/*
 * Class:     org_hime_android_core_HimeEngine
 * Method:    nativeGetInputMode
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_org_hime_android_core_HimeEngine_nativeGetInputMode(JNIEnv *env, jobject thiz) {
    if (g_context == NULL) return HIME_MODE_CHINESE;
    return hime_get_input_mode(g_context);
}
