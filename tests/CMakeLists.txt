add_custom_target(build-tests)

add_custom_target(check
  COMMAND
  ${CMAKE_SOURCE_DIR}/extern/obdev/bin/obdev ts run ${CMAKE_SOURCE_DIR}
  --out ${CMAKE_BINARY_DIR}/obts
  DEPENDS build-tests  
)

set(SRC
  catch_main.cc
)
add_library(catch_main ${SRC})
