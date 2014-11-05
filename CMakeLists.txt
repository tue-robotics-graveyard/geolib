cmake_minimum_required(VERSION 2.8.3)
project(geolib)

find_package(OpenCV REQUIRED)

# ------------------------------------------------------------------------------------------------
#                                                BUILD
# ------------------------------------------------------------------------------------------------

include_directories(
  include
  ${OpenCV_INCLUDE_DIRS}
)

if(EXISTS "/usr/include/assimp/Importer.hpp")
    add_definitions(-DASSIMP_VERSION_3)
endif()

set(HEADER_FILES
    include/geolib/math_types.h
    include/geolib/Ray.h
    include/geolib/serialization.h
    include/geolib/visualization.h
    include/geolib/datatypes.h
    include/geolib/ros/msg_conversions.h
    include/geolib/ros/tf_conversions.h
    include/geolib/Importer.h
    include/geolib/matrix.h
    include/geolib/Triangle.h
    include/geolib/Box.h
    include/geolib/HeightMapNode.h
    include/geolib/Octree.h
    include/geolib/Shape.h
    include/geolib/sensors/LaserRangeFinder.h
    include/geolib/sensors/DepthCamera.h
    include/geolib/Mesh.h
    include/geolib/OctreeNode.h
    include/geolib/CompositeShape.h
    include/geolib/HeightMap.h
)

add_library(geolib
    src/Box.cpp
    src/datatypes.cpp
    src/Octree.cpp
    src/OctreeNode.cpp
    src/Ray.cpp
    src/Shape.cpp
    src/Mesh.cpp
    src/CompositeShape.cpp
    src/Triangle.cpp
    src/HeightMap.cpp
    src/HeightMapNode.cpp
    src/sensors/DepthCamera.cpp
    src/sensors/LaserRangeFinder.cpp
    src/Importer.cpp
    src/serialization.cpp
    src/visualization.cpp                     
    ${HEADER_FILES}
)

target_link_libraries(geolib assimp ${OpenCV_LIBRARIES})

add_executable(show src/show.cpp)
target_link_libraries(show geolib)

# ------------------------------------------------------------------------------------------------
#                                                TOOLS
# ------------------------------------------------------------------------------------------------

add_executable(height-map-to-shape tools/height_image_to_shape.cpp)
target_link_libraries(height-map-to-shape geolib)

# ------------------------------------------------------------------------------------------------
#                                                TESTS
# ------------------------------------------------------------------------------------------------

# add_executable(test_geolib test/test_geolib.cpp)
# target_link_libraries(test_geolib geolib assimp)

# add_executable(test_matrix test/test_matrix.cpp)
