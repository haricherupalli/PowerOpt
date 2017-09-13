##############################################################################
#                                                                            #
#	 Makefile for building PowerOptimizer program --- S.H. Kang, J. Sartori  #
#                                                                            #
##############################################################################

SUFFIX  = cpp
GCC = g++

#OAROOT = /project/sartori00/CAD_SCRIPTS/home/tool/oa/oa2.2.6
OAROOT = /ece/home/cheru007/OpenAccess
#OALIBDIR = $(OAROOT)/lib/linux_rhel30_gcc411_32/opt
OALIBDIR = $(OAROOT)/lib/linux_rhel50_gcc44x_64/dbg
OAINC = $(OAROOT)/include/oa
#OAINC = /project/sartori00/CAD_SCRIPTS/home/tool/oa/oa2.2.6/include/oa
DEBUGEXT =
OALIBS = -loaCommon$(DEBUGEXT) \
         -loaBase$(DEBUGEXT) \
         -loaPlugIn$(DEBUGEXT)\
         -loaDM$(DEBUGEXT)\
         -loaTech$(DEBUGEXT)\
         -loaDesign$(DEBUGEXT)

#PY = purify

TCLLIB = -ltcl8.5
#TCLINC = /home/hailong/Prog/Tcl/include
#TCLLIBPATH = /home/hailong/Prog/Tcl/lib
TCLINC = /usr/include
TCLLIBPATH = /usr/lib64

#IFLAG = -I. -I$(OAINC) -I$(TCLINC) -I/usr/include/c++/3.4.6/backward/ -I /usr/include/linux/
IFLAG = -I. -I$(OAINC) -I$(TCLINC) -I/usr/local/apps/00/01/common/gcc/4.2.0/Linux/x86_64/include/c++/4.2.0/backward/
#IFLAG = -I. -I$(OAINC) -I$(TCLINC)
LFLAG = -fPIC $(DBG) -L$(OALIBDIR)  -L$(TCLLIBPATH) $(TCLLIB) -lm -lpthread -ldl -Wl,-R/lib

BINPATH = ../bin
OBJPATH = ../obj

SOURCES = $(wildcard *.$(SUFFIX))
OBJECTS = $(SOURCES:%.$(SUFFIX)=$(OBJPATH)/%.o)
HEADERS = $(wildcard ./*.h)
CFLAGS = $(IFLAG) -I$(TOOLSDIR)/include/oa \
         -I$(TOOLSDIR)/include \
         -c -fPIC -Wno-deprecated $(DBG) -O0

#-std=c++0x -std=gnu++0x

TARGET = $(BINPATH)/PowerOpt$(EXPER)

$(TARGET):$(OBJECTS)
	@echo "Now Generating $(TARGET)"
	$(PY) $(GCC)  -o $(TARGET)  $(IFLAG) \
	$(OBJECTS) \
	$(LFLAG)
	@echo "Done!"

# Compile the application code
$(OBJPATH)/%.o: %.$(SUFFIX) $(HEADERS)
	@echo "Now Compile $< ..."
	$(PY) $(GCC) -o $@ $(CFLAGS) $<
	@echo "Done!"
.KEEP_STATE:
clean:
	@echo "Now Removing ..."
	@-rm -rf $(OBJECTS)
	#@-rm -rf $(BINPATH)/$(TARGET)
	@echo "Done!"
debug:
	@$(MAKE) -f makefile DBG="-g -O0"
profile:
	@$(MAKE) -f makefile DBG="-pg"
opt:
	@$(MAKE) -f makefile DBG="-O3"
test:
	@$(MAKE) -f makefile DBG="-DTEST" EXPER="_test"
timing:
	@$(MAKE) -f makefile DBG="-DTIMING" EXPER="_timing"
lsmc:
	@$(MAKE) -f makefile DBG="-DLSMC" EXPER="_lsmc"
exp1:
	@$(MAKE) -f makefile DBG="-DEXP1" EXPER="_exp1"
exp2:
	@$(MAKE) -f makefile DBG="-DEXP2" EXPER="_exp2"
exp3:
	@$(MAKE) -f makefile DBG="-DEXP3" EXPER="_exp3"
exp4:
	@$(MAKE) -f makefile DBG="-DEXP4" EXPER="_exp4"
exp5:
	@$(MAKE) -f makefile DBG="-DEXP5" EXPER="_exp5"
exp6:
	@$(MAKE) -f makefile DBG="-DEXP6" EXPER="_exp6"
exp7:
	@$(MAKE) -f makefile DBG="-DEXP7" EXPER="_exp7"
exp1_3:
	@$(MAKE) -f makefile DBG="-DEXP1_3" EXPER="_exp1_3"
exp1_4:
	@$(MAKE) -f makefile DBG="-DEXP1_4" EXPER="_exp1_4"
exp2_3:
	@$(MAKE) -f makefile DBG="-DEXP2_3" EXPER="_exp2_3"
exp3_4:
	@$(MAKE) -f makefile DBG="-DEXP3_4" EXPER="_exp3_4"
slackdist:
	@$(MAKE) -f makefile EXPER="_slackdist"
var:
	@$(MAKE) -f makefile DBG="-g" EXPER="_var"
explain:
	@echo "The following information represents your program:"
	@echo "Final executable name: $(TARGET)"
	@echo "Source files: $(SOURCES)"
	@echo "Object files: $(OBJECTS)"
