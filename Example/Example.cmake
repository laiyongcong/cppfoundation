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

set(${PROJECT_NAME}_All_Sources
	${Source_Files_Sources}
	${Header_Files_Sources}
)
set(${PROJECT_NAME}_Local_Include_Dirs
	./
	Generated
)