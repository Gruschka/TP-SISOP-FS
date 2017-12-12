RM=rm -rf
CC=gcc

C_SRCS=$(shell find . -iname "*.c" | tr '\n' ' ')
H_SRCS=$(shell find . -iname "*.h" | tr '\n' ' ')
OBJS=$(C_SRCS:./%.c=build/%.o)

# Clean and compile .so
all: build/libipc.so

create-dirs:
	mkdir -p build/ipc

build/libipc.so: create-dirs $(OBJS)
	$(CC) -shared -Wall -o "build/libipc.so" $(OBJS)

build/ipc/%.o: ipc/%.c
	$(CC) -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"

# Add debug parameters and compile
debug: CC += -DDEBUG -g
debug: all

# Clean build files
clean:
	$(RM) build

install: all
	cp -u build/libipc.so /usr/lib
	cp --parents -u $(H_SRCS) /usr/include

uninstall:
	rm -f /usr/lib/libipc.so
	rm -rf /usr/include/lib-ipc

.PHONY: all create-dirs clean install uninstall
