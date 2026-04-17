#pragma once

#include <Arduino.h>

#ifndef PRODUCT_VERSION
#define PRODUCT_VERSION 1
#endif

#ifndef APP_VERSION
#define APP_VERSION "202"
#endif

#ifndef APP_BUILD_NUMBER
#define APP_BUILD_NUMBER 0
#endif

namespace Version {

constexpr int kProductVersion = PRODUCT_VERSION;
constexpr const char* kEspVersion = APP_VERSION;
constexpr int kBuildNumber = APP_BUILD_NUMBER;

inline int espVersionInt() {
    return atoi(APP_VERSION);
}

inline String makeDeviceVersion(int nextionVersion,
                                int espVersion = espVersionInt(),
                                int buildNumber = APP_BUILD_NUMBER,
                                int productVersion = PRODUCT_VERSION) {
    return String(productVersion) + "." +
           String(nextionVersion < 0 ? 0 : nextionVersion) + "." +
           String(espVersion) + "." +
           String(buildNumber);
}

inline String makeDeviceVersionFromNextionText(const String& nextionText) {
    return makeDeviceVersion(nextionText.toInt());
}

} // namespace Version
