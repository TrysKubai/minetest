project(MTSerializationInterface)


set(SERIALIZATION_INCLUDES
        ${CMAKE_SOURCE_DIR}/serialization_interface
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/lib/irrlichtmt/include)

add_library(${PROJECT_NAME} STATIC serializationInterface.cpp) #${SERIALIZATION_SRCS})
target_include_directories(${PROJECT_NAME} PUBLIC ${SERIALIZATION_INCLUDES})
# target_link_libraries(${PROJECT_NAME} IrrlichtMt::IrrlichtMt )

set_target_properties(${PROJECT_NAME} PROPERTIES
	LIBRARY_OUTPUT_DIRECTORY  "${CMAKE_SOURCE_DIR}/out/serialization_interface")

