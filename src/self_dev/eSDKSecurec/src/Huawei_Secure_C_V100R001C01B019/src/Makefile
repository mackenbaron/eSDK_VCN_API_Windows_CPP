PROJECT=libsecurec.so
#if you need a debug version library, use "-g" instead of "-s -DNDEBUG -O2".
CFLAG= -I ../include -Wall  -s -DNDEBUG -O2
CXXFLAG= $(CFLAG)
CC=gcc
GCC=gcc
ARCH:=$(shell getconf LONG_BIT)

#Security compiler options
#stack protector
STACK_PRO = -fstack-protector --param ssp-buffer-size=4
#GOT protector
GOT_PRO = -Wl,-z,relro,-z,now
#Dynamic library search path
#Stack cannot be executed
STACK_NOTEXE_PRO = -Wl,-z,noexecstack
#Address independent
ADD_IND = -fPIC
CFLAG += $(STACK_PRO) $(GOT_PRO) $(STACK_NOTEXE_PRO) $(ADD_IND)
CXXFLAG += $(STACK_PRO) $(GOT_PRO) $(STACK_NOTEXE_PRO) $(ADD_IND)

ifeq ($(os), v100nptl)
    CC=/opt/hisi-linux-nptl/arm-hisiv100-linux/target/bin/arm-hisiv100nptl-linux-gcc
    GCC=/opt/hisi-linux-nptl/arm-hisiv100-linux/target/bin/arm-hisiv100nptl-linux-gcc
endif

SOURCES=$(wildcard *.c)
OBJECTS=$(patsubst %.c,%.o,$(SOURCES))

$(PROJECT):$(OBJECTS)
	@mkdir -p ../obj
	mkdir -p ../lib
	$(GCC)  -shared -o ../lib/$@ $(patsubst %.o,../obj/%.o,$^) $(CFLAG) 
	#cp ../lib/libsecurec.so /usr/local/lib/libsecurec.so
	#you may add you custom commands here
	@echo "finish $(PROJECT)"
.c.o:
	@mkdir -p ../obj
	$(GCC) -c $< $(CFLAG) -o ../obj/$(patsubst %.c,%.o,$<)

clean:
	rm -rf *.o ../obj ../lib $(PROJECT)

