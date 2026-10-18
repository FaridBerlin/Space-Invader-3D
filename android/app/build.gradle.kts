// Build settings for the Android app.
//
// There is no Kotlin or Java code in this app. The whole game is our C++ code,
// built by the SAME CMakeLists.txt as the desktop and web versions (two folders
// up). Gradle runs CMake with the Android NDK, packs the result
// (libspace-invader-3d.so) into the app, and Android's built-in NativeActivity
// starts it (see AndroidManifest.xml).

plugins {
    alias(libs.plugins.android.application)
}

android {
    namespace = "com.faridberlin.spaceinvader3d"
    compileSdk {
        version = release(37)
    }
    ndkVersion = "30.0.16248370"   // the NDK installed in Android Studio's SDK Manager

    defaultConfig {
        applicationId = "com.faridberlin.spaceinvader3d"   // never change this after the first Play upload
        minSdk = 24          // Android 7.0 and newer
        targetSdk = 37
        versionCode = 1      // raise by 1 for every upload to Google Play
        versionName = "1.0"  // the version people see

        ndk {
            // arm64-v8a: practically every phone sold in the last years.
            // x86_64:    the Android Studio emulator on a normal PC.
            abiFilters += listOf("arm64-v8a", "x86_64")
        }
    }

    externalNativeBuild {
        cmake {
            path = file("../../CMakeLists.txt")   // the game's main CMakeLists.txt
            version = "3.22.1"
        }
    }

    // The game's sounds and music: use the repo's assets folder directly, so
    // there is only one copy of every file.
    sourceSets {
        getByName("main") {
            assets.srcDir("../../assets")
        }
    }
    androidResources {
        // Files in assets/ that must NOT go into the app ("!" = leave out):
        // the local 3D model (its license is unclear) and the README notes.
        ignoreAssetsPatterns += listOf("!*.glb", "!*.md")
    }

    buildTypes {
        release {
            isMinifyEnabled = false   // there is no Java/Kotlin code to shrink
        }
    }
}
