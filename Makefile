CPPC=g++

CFLAGS=-g -O0 -DVK_USE_PLATFORM_XCB_KHR -Wall -Werror
LD_FLAGS=-lvulkan -lxcb


OBJECTS=vulkan-core.o
MAIN_OBJECTS=triangle.o
BINARIES=triangle

SHADERS=triangle.vert triangle.frag
SHADERS_OBJECTS=$(SHADERS:=.spv)

DEPENDENCY_RULES=$(OBJECTS:=.d) $(MAIN_OBJECTS:=.d)

all: shaders triangle

triangle: triangle.o $(OBJECTS)
	$(CPPC) $(LD_FLAGS) $^ -o $@

shaders: $(SHADERS_OBJECTS)

%.spv: %
	glslangValidator -V $< -o $@ &>/dev/null

%.o: %.cc
	$(CPPC) $(CFLAGS) -c $< -o $@
	@$(CROSS_COMPILE)$(CXX) $(CXXFLAGS) -MM $< > $@.d

-include $(DEPENDENCY_RULES)

clean:
	rm -rf $(BINARIES) *.o *.spv *.d

.PHONY: all
