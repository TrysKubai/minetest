#pragma once

#include "mapblock.h"

#ifdef SERIALIZATION_API_LIB
    #ifdef WIN32
        #define SERIALIZATION_API __declspec(dllexport)
    #else
        #define SERIALIZATION_API __attribute__ ((visibility ("default")))
    #endif
#else
    #ifdef WIN32
        #define SERIALIZATION_API __declspec(dllimport)
    #else
        #define SERIALIZATION_API
    #endif
#endif

namespace SerializationInterface
{
    SERIALIZATION_API MapBlock deserialize(const char*);
    SERIALIZATION_API char* serialize(const MapBlock&);
}


