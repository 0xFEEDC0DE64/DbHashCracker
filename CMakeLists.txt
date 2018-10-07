project(DbHashCracker)

set(HEADERS
    workerthread.h
)

set(SOURCES
    main.cpp
    workerthread.cpp
)

add_executable(hashcracker ${HEADERS} ${SOURCES})

target_link_libraries(hashcracker Qt5::Core Qt5::Sql)
