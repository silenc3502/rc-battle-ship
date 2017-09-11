PHONY: all

all: barcode-roi-detect
LIBS = -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_ml -lopencv_video -lopencv_features2d -lopencv_calib3d -lopencv_objdetect -lopencv_imgcodecs -lopencv_video -lopencv_videoio

barcode-roi-detect: detect_barcode.cpp
	$(CXX) detect_barcode.cpp -o detect_barcode $(LIBS) $(CXXFLAGS)

clean:
	rm -rf detect_barcode

