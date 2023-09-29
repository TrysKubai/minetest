#define SERIALIZATION_API_LIB

#include "serializationInterface.h"

#include <iostream>
#include <string>

MapBlock SerializationInterface::deserialize(const char* buff)
{
    std::cout<<"Deserialization Called"<<std::endl;
    return MapBlock(NULL, v3s16(0,0,0), NULL);
}

char* SerializationInterface::serialize(const MapBlock &mapBlock)
{
    std::cout<<"Serialization Called"<<std::endl;
    return NULL;
}