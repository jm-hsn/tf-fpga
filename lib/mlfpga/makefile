# Object files to either reference or create                          
OBJECTS = main.o udp.o conv2d.o dummy.o job.o modules.o connectionManager.o
# The executable file that will be created at the end                 
EXEC = proj.out 
# The flags to use for compilation                                    
FLAGS = -g -Wall -pthread -std=c++11
# The code compiler to use for compilation                            
CC = /usr/bin/g++


CV = `pkg-config opencv --cflags --libs`
CV2 = -I/usr/include/opencv2 -lopencv_shape -lopencv_stitching -lopencv_superres -lopencv_videostab -lopencv_aruco -lopencv_bgsegm -lopencv_bioinspired -lopencv_ccalib -lopencv_datasets -lopencv_dpm -lopencv_face -lopencv_freetype -lopencv_fuzzy -lopencv_hdf -lopencv_line_descriptor -lopencv_optflow -lopencv_video -lopencv_plot -lopencv_reg -lopencv_saliency -lopencv_stereo -lopencv_structured_light -lopencv_phase_unwrapping -lopencv_rgbd -lopencv_viz -lopencv_surface_matching -lopencv_text -lopencv_ximgproc -lopencv_calib3d -lopencv_features2d -lopencv_flann -lopencv_xobjdetect -lopencv_objdetect -lopencv_ml -lopencv_xphoto -lopencv_highgui -lopencv_videoio -lopencv_imgcodecs -lopencv_photo -lopencv_imgproc -lopencv_core


# Search for .cpp files in "src" directory; .hpp files in "include" directory
# The pattern matching character '%' matches filename without the extension
vpath %.cpp src
vpath %.hpp include

all: $(EXEC)

$(EXEC): $(OBJECTS)
	$(CC) $(FLAGS) -o $@ $(OBJECTS) $(CV)


%.o: %.cpp %.hpp
	$(CC) -c $(FLAGS) $<

clean:
	rm -f $(OBJECTS) $(EXEC)
