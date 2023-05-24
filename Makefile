CPPFLAGS := $(INC_FLAGS) -Bstatic
LDFLAGS := -lssl -lcrypto -ldl -pthread

.SILENT: all xpkey srv2003key clean

all: xpkey srv2003key

xpkey:
	$(CXX) $(CPPFLAGS) main.cpp -o $@ $(LDFLAGS)

srv2003key:
	$(CXX) $(CPPFLAGS) Srv2003KGmain.cpp -o $@ $(LDFLAGS)

clean:
	rm -f xpkey srv2003key