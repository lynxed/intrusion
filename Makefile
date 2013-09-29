CC = clang
NASM = nasm
STRIP = sstrip -z
SHADER_MINIFIER = shader_minifier.exe
CFLAGS = -m32 -std=c99 -Wall -Werror -ggdb -O0 $(shell pkg-config --cflags sdl) $(shell pkg-config --cflags gl) $(shell pkg-config --cflags glew)
LDFLAGS = -m32 -lm -lpthread $(shell pkg-config --libs sdl) $(shell pkg-config --libs gl) $(shell pkg-config --libs glew)
NASMFLAGS = -f elf

SOURCES = $(wildcard *.c)
ASM_SOURCES = $(wildcard *.asm)
OBJECTS = $(SOURCES:.c=.o) $(ASM_SOURCES:.asm=.o)
SHADERS = $(wildcard *.glsl)
SHADER_HEADER = shader_code.h
EXECUTABLE = planeshift

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) $(MAKEFILE_LIST)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS)
	$(STRIP) $@

$(SHADER_HEADER): $(SHADERS)
	$(SHADER_MINIFIER) --preserve-externals -o $@ $^

%.o: %.asm $(MAKEFILE_LIST)
	$(NASM) $(NASMFLAGS) $< -o $@

%.o: %.c %.d $(MAKEFILE_LIST) $(SHADER_HEADER)
	$(CC) -c $(CFLAGS) $< -o $@

%.d: %.c $(SHADER_HEADER)
	@$(SHELL) -ec '$(CC) -M $(CFLAGS) $< \
		| sed '\''s/\($*\)\.o[ :]*/\1.o $@ : /g'\'' > $@; \
		[ -s $@ ] || rm -f $@'

clean:
	rm -rf *.o *.d $(EXECUTABLE) $(SHADER_HEADER)

-include $(SOURCES:.c=.d)

