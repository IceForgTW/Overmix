cmake_minimum_required(VERSION 3.0)

find_package(Qt5Gui REQUIRED)
find_package(Qt5Concurrent REQUIRED)

# Turn on warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

set(SOURCES_CONTAINERS
	containers/AContainer.cpp
	containers/ImageContainer.cpp
	containers/ImageContainerSaver.cpp
	)

set(SOURCES_PLANES
	planes/ColorSpace.cpp
	planes/DistanceMatrix.cpp
	planes/ImageEx.cpp
	planes/ImageEx-dump.cpp
	planes/ImageEx-jpeg.cpp
	planes/ImageEx-png.cpp
	planes/ImageEx-qimage.cpp
	planes/FourierPlane.cpp
	planes/Plane.cpp
	planes/Plane-scaling.cpp
	planes/Plane-edgedetection.cpp
	planes/Plane-blurring.cpp
	planes/Plane-diff.cpp
	planes/Plane-pixel.cpp
	planes/Plane-binarize.cpp
	planes/PatternRemove.cpp
	planes/gwenview/iodevicejpegsourcemanager.cpp
	)
	
set(SOURCES_ALIGNERS
	aligners/AImageAligner.cpp
	aligners/AverageAligner.cpp
	aligners/ClusterAligner.cpp
	aligners/RecursiveAligner.cpp
	aligners/AnimationSeparator.cpp
	aligners/FrameAligner.cpp
	aligners/LinearAligner.cpp
	aligners/SuperResAligner.cpp
	)
	
set(SOURCES_COMPARATORS
	comparators/GradientComparator.cpp
	)
	
set(SOURCES_RENDERS
	renders/AnimRender.cpp
	renders/AverageRender.cpp
	renders/FloatRender.cpp
	renders/DiffRender.cpp
	renders/DistanceMatrixRender.cpp
	renders/PlaneRender.cpp
	renders/StatisticsRender.cpp
	renders/PixelatorRender.cpp
	renders/RobustSrRender.cpp
	renders/EstimatorRender.cpp
	renders/JpegRender.cpp
	)
	
set(SOURCES_DEGRADERS
	degraders/JpegDegrader.cpp
	)
	
set(SOURCES_OTHERS
	aligners/AnimationSaver.cpp
	planes/dump/DumpPlane.cpp
	utils/Animator.cpp
	utils/ImageLoader.cpp
	color.cpp
	debug.cpp
	Deteleciner.cpp
	MultiPlaneIterator.cpp
	)

# mingw uses a different naming for FFTW apparently # TODO: find a better solution
if( WIN32 )
	set(PLATFORM_LIBRARIES ${PLATFORM_LIBRARIES} -lfftw3-3)
else( WIN32 )
	set(PLATFORM_LIBRARIES ${PLATFORM_LIBRARIES} -lfftw3)
endif( WIN32 )

add_library(OvermixCore ${SOURCES_CONTAINERS} ${SOURCES_PLANES} ${SOURCES_ALIGNERS} ${SOURCES_COMPARATORS} ${SOURCES_RENDERS} ${SOURCES_DEGRADERS} ${SOURCES_OTHERS} )
target_link_libraries(OvermixCore Qt5::Gui Qt5::Concurrent -lm -lpng -ljpeg -lz -llzma -lpugixml ${PLATFORM_LIBRARIES})

# C++14
set_property(TARGET OvermixCore PROPERTY CXX_STANDARD 14)
set_property(TARGET OvermixCore PROPERTY CXX_STANDARD_REQUIRED ON)
