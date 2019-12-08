#pragma once

#ifdef PLATFORM_WINDOWS
    #pragma warning( push )
    #pragma warning( disable : 4251)
    #pragma warning( disable : 4275)
#endif // PLATFORM_WINDOWS

#include <yaml-cpp/yaml.h>

#ifdef PLATFORM_WINDOWS
    #pragma warning( pop ) 
#endif // PLATFORM_WINDOWS