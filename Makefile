All : SS

CXX = g++
FLAGS = -std=c++11 -Wall -DBUILD_PNG=ON -DWITH_PNG=ON
BIN = ./Sprite-Splitter
#INCLUDE = -I/usr/local/include/boost/ -I/usr/local/include/opencv4/opencv2 -I/usr/local/include/opencv4/
INCLUDE = -I/usr/include/ -I/usr/local/include/ -I/usr/local/include/opencv4/
LIB = -L/usr/lib/ -L/usr/local/lib/
BOOST_LIBS = -lboost_system -lboost_filesystem -lboost_program_options
OPENCV_LIBS = -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_stitching -lopencv_imgcodecs

MWXX = x86_64-w64-mingw32-g++
MWFLAGS = -std=c++11 -Wall -DBUILD_PNG=ON -DWITH_PNG=ON
MWBIN = ./Sprite-Splitter.exe
MWLIB = -L/usr/x86_64-w64-mingw32/sys-root/mingw/lib/
MWLIB_FLAGS = -lSDL2 -lSDL2main -lmingw32
MWINCLUDE = -I/usr/x86_64-w64-mingw32/sys-root/mingw/include/

SS : ./program.cpp
	@$(CXX) $(FLAGS) -o $(BIN) $(INCLUDE) $(LIB) $(BOOST_LIBS) $(OPENCV_LIBS) $^
	@echo "Generating $@ ..."

MinGW : ./program.cpp
	@$(MWGXX) $(MWFLAGS) -o $(MWBIN) $(INCLUDE) -I/usr/x86_64-w64-mingw32/sys-root/mingw/include/ $(LIB) $(BOOST_LIBS) $(OPENCV_LIBS) $^
	@echo "Generating $@ ..."


SS_win : ./program.cpp
	@$(CXX) $(MWFLAGS) -o $(MWBIN) $(MWINCLUDE) $(MWLIB) $(BOOST_LIBS) $(OPENCV_LIBS) $(MWLIB_FLAGS) $^
	@echo "Generating $@ ..."