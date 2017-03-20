CPPC=g++

CFLAGS=-g -O0 -DVK_USE_PLATFORM_XCB_KHR -Wall -Werror
LD_FLAGS=-lvulkan -lxcb


OBJECTS=vulkan-core.o
MAIN_OBJECTS=triangle.o
BINARIES=triangle

DEPENDENCY_RULES=$(OBJECTS:=.d) $(MAIN_OBJECTS:=.d)

triangle: triangle.o $(OBJECTS)
	$(CPPC) $(LD_FLAGS) $^ -o $@

%.o: %.cc
	$(CPPC) $(CFLAGS) -c $< -o $@
	@$(CROSS_COMPILE)$(CXX) $(CXXFLAGS) -MM $< > $@.d

-include $(DEPENDENCY_RULES)

clean:
	rm -rf $(BINARIES) *.o
