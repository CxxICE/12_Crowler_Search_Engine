cmake_minimum_required(VERSION 3.20)

add_executable(crowler
	"main.cpp"
	"${SRC_DIR}/crowler/crowler.cpp"
	"${SRC_DIR}/crowler_database/crowler_database.cpp"
	"${SRC_DIR}/parser_ini_file/parser_ini_file.cpp"
	"${SRC_DIR}/safe_queue/safe_queue.cpp"
	"${SRC_DIR}/thread_pool/thread_pool.cpp"
	)

target_compile_features(crowler PRIVATE cxx_std_20) 

target_include_directories(crowler PRIVATE ${Boost_INCLUDE_DIRS} ${SRC_DIR} ${PQXX_DIR} ${SSL_DIR})
target_link_directories(crowler PRIVATE ${PQXX_LINK_DIR} ${SSL_LINK_DIR} ${PSQL_LINK_DIR})

target_link_libraries(crowler pqxx)
target_link_libraries(crowler libcrypto_static libssl_static)
target_link_libraries(crowler libpq)
target_link_libraries(crowler ${Boost_LIBRARIES})

add_custom_command(TARGET crowler POST_BUILD
					COMMAND ${CMAKE_COMMAND} -E copy
					"${CMAKE_CURRENT_SOURCE_DIR}/crowler.ini" "$<TARGET_FILE_DIR:crowler>")
					
add_custom_command(TARGET crowler POST_BUILD
					COMMAND ${CMAKE_COMMAND} -E copy
					${DLL_COPY} "$<TARGET_FILE_DIR:crowler>")
