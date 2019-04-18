/*
** Copyright (c) 2016 CognitiveVR, Inc. All rights reserved.
*/
#ifndef COGNITIVEVR_UTIL_H_
#define COGNITIVEVR_UTIL_H_

#include "CognitiveVR.h"
#include <sstream>
#include <ctime>
#include <string>
#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>
#include "Json.h"
#include "Windows/WindowsPlatformMisc.h"

namespace cognitivevrapi
{
    class Util
    {
        public:
            static double GetTimestamp();

			static FString GetDeviceName(FString DeviceName);

			//record several default hardware values to session properties
			static void SetHardwareSessionProperties();
    };
}
#endif  // COGNITIVEVR_UTIL_H_