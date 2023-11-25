
aux_source_directory(. coio_src)
add_library(coio STATIC  ${coio_src})

target_include_directories(coio PUBLIC .)
target_link_libraries(coio PUBLIC common)

