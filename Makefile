CC      = gcc
CPPFLAGS  = -Wall -g -std=c17 -pedantic
.SUFFIXES:	.d .c .o

.PHONY:	default clean style

default:	Example

Example:	Example.o ThreadPool.o ConcurrencyUtil.o -lm -lpthread

.c.d:
	@set -e; rm -f $@; \
	$(CC) -MM $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

sources = $(wildcard *.c)

include $(sources:.c=.d)

clean:
	rm -f $(wildcard *.o)
	rm -f $(wildcard *.d)
	rm -f Example

style:
	clang-format -style=file -i *.c *.h
