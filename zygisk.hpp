name: Build Zygisk Spoof SO
on:
  push:
    branches: [ main ]
  workflow_dispatch:

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - name: Checkout code
        uses: actions/checkout@v4

      - name: Download Android NDK r26c (Google Official CDN)
        run: |
          set -e
          wget -q --timeout=120 --tries=5 https://dl.google.com/android/repository/android-ndk-r26c-linux.zip
          unzip -q android-ndk-r26c-linux.zip
          echo "NDK_ROOT=$PWD/android-ndk-r26c" >> $GITHUB_ENV
          ls -la android-ndk-r26c

      - name: Build arm64-v8a Zygisk lib
        run: |
          ${{ env.NDK_ROOT }}/ndk-build \
            NDK_PROJECT_PATH=. \
            APP_BUILD_SCRIPT=jni/Android.mk \
            APP_PLATFORM=android-31 \
            APP_ABI=arm64-v8a

      - name: Upload compiled lib artifact
        uses: actions/upload-artifact@v4
        with:
          name: zygisk-spoof-lib-arm64
          path: libs/arm64-v8a/libzygisk.so
