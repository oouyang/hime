# HIME Android ProGuard Rules

# Keep JNI methods
-keepclasseswithmembernames class * {
    native <methods>;
}

# Keep HimeEngine JNI interface
-keep class org.hime.android.core.HimeEngine {
    native <methods>;
    *;
}

# Keep IME service
-keep class org.hime.android.ime.HimeInputMethodService {
    *;
}

# Keep custom views
-keep class org.hime.android.view.** {
    *;
}
