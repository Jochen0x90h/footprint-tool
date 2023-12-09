add_executable(${PROJECT_NAME}
	main.cpp
	clipper2.hpp
	double2.hpp
	double3.hpp
)
target_link_libraries(${PROJECT_NAME}
	nlohmann_json::nlohmann_json
	PkgConfig::Clipper2
)

# install
install(TARGETS ${PROJECT_NAME}
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	RUNTIME DESTINATION bin
)
