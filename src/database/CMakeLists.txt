#This file has been modified by Three Cubes
###################
#	Changelog
###################
#
# 2023-06-14(@Shumeras): Removed old 'files' and 'leveldb' database types 

set(database_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/database.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/database-dummy.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/database-postgresql.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/database-redis.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/database-sqlite3.cpp
	PARENT_SCOPE
)
