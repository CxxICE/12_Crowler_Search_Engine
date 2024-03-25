cmake_minimum_required(VERSION 3.20)

add_executable(search_engine
	"main.cpp"	
	"${SRC_DIR}/http_server_coro/http_server_coro.cpp"
	"${SRC_DIR}/crowler_database/crowler_database.cpp"
	"${SRC_DIR}/parser_ini_file/parser_ini_file.cpp"
	)

target_compile_features(search_engine PRIVATE cxx_std_20) 

target_include_directories(search_engine PRIVATE ${Boost_INCLUDE_DIRS} ${SRC_DIR} ${PQXX_DIR})
target_link_directories(search_engine PRIVATE ${PQXX_LINK_DIR} ${PSQL_LINK_DIR})

target_link_libraries(search_engine pqxx)
target_link_libraries(search_engine libpq)
target_link_libraries(search_engine ${Boost_LIBRARIES})

add_custom_command(TARGET search_engine POST_BUILD
					COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${CMAKE_CURRENT_SOURCE_DIR}/html_css/" "$<TARGET_FILE_DIR:search_engine>/html_css/")

add_custom_command(TARGET search_engine POST_BUILD
					COMMAND ${CMAKE_COMMAND} -E copy
					"${CMAKE_CURRENT_SOURCE_DIR}/search_engine.ini" "$<TARGET_FILE_DIR:search_engine>/")
					
add_custom_command(TARGET search_engine POST_BUILD
					COMMAND ${CMAKE_COMMAND} -E copy
					${DLL_COPY} "$<TARGET_FILE_DIR:search_engine>/")