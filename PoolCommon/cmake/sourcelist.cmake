if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC" OR "${CMAKE_CXX_COMPILER_ID};${CMAKE_CXX_COMPILER_FRONTEND_VARIANT}" STREQUAL "Clang;MSVC")

set(SOURCE_FILES
	"${SOURCE_DIR}/ArrayBuffer.cpp"
	"${SOURCE_DIR}/BufferOperations.cpp"
	"${SOURCE_DIR}/Data.cpp"
	"${SOURCE_DIR}/Execution.cpp"
	"${SOURCE_DIR}/ExecutionHandler.cpp"
	"${SOURCE_DIR}/Flags.cpp"
	"${SOURCE_DIR}/Function.cpp"
	"${SOURCE_DIR}/IPCommManager.cpp"
	"${SOURCE_DIR}/Input.cpp"
	"${SOURCE_DIR}/Logging.cpp" 
	"${SOURCE_DIR}/LZMAStreambuf.cpp" 
	"${SOURCE_DIR}/PCH.h"
	"${SOURCE_DIR}/Processes.cpp"
	"${SOURCE_DIR}/Serializable.cpp"
	"${SOURCE_DIR}/TaskController.cpp"
	"${SOURCE_DIR}/Utility.cpp"
	"${SOURCE_DIR}/Version.cpp"
)
else()

set(SOURCE_FILES
	"${SOURCE_DIR}/ArrayBuffer.cpp"
	"${SOURCE_DIR}/BufferOperations.cpp"
	"${ROOT_DIR}/DeathHandler/death_handler.cc"
	"${SOURCE_DIR}/Data.cpp"
	"${SOURCE_DIR}/Execution.cpp"
	"${SOURCE_DIR}/ExecutionHandler.cpp"
	"${SOURCE_DIR}/Flags.cpp"
	"${SOURCE_DIR}/Function.cpp"
	"${SOURCE_DIR}/Evaluation.cpp"
	"${SOURCE_DIR}/IPCommManager.cpp"
	"${SOURCE_DIR}/Input.cpp"
	"${SOURCE_DIR}/Logging.cpp" 
	"${SOURCE_DIR}/LZMAStreambuf.cpp" 
	"${SOURCE_DIR}/PCH.h"
	"${SOURCE_DIR}/Processes.cpp"
	"${SOURCE_DIR}/Serializable.cpp"
	"${SOURCE_DIR}/TaskController.cpp"
	"${SOURCE_DIR}/Utility.cpp"
	"${SOURCE_DIR}/Version.cpp"
)

endif()
