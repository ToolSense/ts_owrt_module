# These variables hold the name of the compilation tool, the compilation flags and the link flags
# We make use of these variables in the package manifest
TARGET = ts_owrt_module  
CC = gcc
CFLAGS = -Wall  -I/usr/include/modbus
OBJDIR = ../bin
LIBS=-lmodbus -lmosquitto -lconfig

# Global target; when 'make' is run without arguments, this is what it should do
all: $(OBJDIR) $(OBJDIR)/$(TARGET)

# This variable identifies all header files in the directory; we use it to create a dependency chain between the object files and the source files
# This approach will re-build your application whenever any header file changes. In a more complex application, such behavior is often undesirable
DEPS = $(wildcard *.h)
 
# This variable holds all source files to consider for the build; we use a wildcard to pick all files
SRC = $(wildcard *.c)
 
# This variable holds all object file names, constructed from the source file names using pattern substitution
OBJ = $(addprefix $(OBJDIR)/, $(patsubst %.c, %.o, $(SRC)))
 
# This rule to add directory for .o
$(OBJDIR):
	mkdir $(OBJDIR)

# This rule builds individual object files, and depends on the corresponding C source files and the header files
$(OBJDIR)/%.o: %.c $(DEPS)
		$(CC) $(CFLAGS) -c $< -o $@

# To build 'ts_owrt_module', we depend on the object files, and link them all into a single executable using the compilation tool
# We use automatic variables to specify the final executable name 'ts_owrt_module', using '$@' and the '$^' will hold the names of all the
# dependencies of this rule
$(OBJDIR)/$(TARGET): $(OBJ)
		$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)
 
# To clean build artifacts, we specify a 'clean' rule, and use PHONY to indicate that this rule never matches with a potential file in the directory
.PHONY: clean
 
clean:
		rm -f $(OBJDIR)/*.o
		rm -f $(OBJDIR)/$(TARGET)