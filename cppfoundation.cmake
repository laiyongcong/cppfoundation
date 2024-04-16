FILE(GLOB Source_Files_Sources "./*.c*")
source_group(Source\ Files
	FILES
	${Source_Files_Sources}
)

FILE(GLOB Header_Files_Sources "./*.h")
source_group(Header\ Files
	FILES
	${Header_Files_Sources}
)

FILE(GLOB Reflection_Files_Sources "Reflection/*.*")
source_group(Reflection\ Files
	FILES
	${Reflection_Files_Sources}
)

FILE(GLOB Threading_Files_Sources "Threading/*.*")
source_group(Threading\ Files
	FILES
	${Threading_Files_Sources}
)

FILE(GLOB Utils_Files_Sources "Utils/*.*")
source_group(Utils\ Files
	FILES
	${Utils_Files_Sources}
)

FILE(GLOB Fasterjson_Files_Sources "Fasterjson/*.*")
source_group(Fasterjson\ Files
	FILES
	${Fasterjson_Files_Sources}
)

set(${PROJECT_NAME}_All_Sources
	${Source_Files_Sources}
	${Header_Files_Sources}
	${Reflection_Files_Sources}
	${Threading_Files_Sources}
	${Utils_Files_Sources}
	${Fasterjson_Files_Sources}
)
set(${PROJECT_NAME}_Local_Include_Dirs
	./
	Reflection
	Threading
	Utils
	Fasterjson
)