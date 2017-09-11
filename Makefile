OSV_SRC=../..
LDFLAGS=-lpthread
CPPLAGS += -std=c++11
## CPPLAGS += -I$(OSV_SRC) -I$(OSV_SRC)/include/ -I$(OSV_SRC)/arch/x64

CPP=c++ $(CPPLAGS) $(LDFLAGS) -ggdb
CPPSO=$(CPP) -fPIC -shared 
GCC=gcc $(LDFLAGS) -ggdb
GCCSO=$(GCC) -fPIC -shared 

MYEXE=httpclient

all: $(MYEXE) $(MYEXE).so
clean:
	rm -f $(MYEXE) $(MYEXE).so

$(MYEXE): $(MYEXE).cc
	$(CPP) -o $@ $<

$(MYEXE).so: $(MYEXE).cc
	$(CPPSO) -o $@ $<


module: all

#
