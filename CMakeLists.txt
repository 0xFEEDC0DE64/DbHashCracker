project(DbHashCracker)

find_package(Qt5Core CONFIG REQUIRED)
find_package(Qt5Sql CONFIG REQUIRED)

set(HEADERS
    workerthread.h
)

set(SOURCES
    main.cpp
    workerthread.cpp
)

add_executable(hashcracker ${HEADERS} ${SOURCES})

target_link_libraries(hashcracker stdc++ Qt5::Core Qt5::Sql)
