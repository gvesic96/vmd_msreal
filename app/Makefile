sources=$(wildcard *.c)
objs=$(sources:.c=.o)

all: main 
test_image: $(objs)
	@echo "Building output binary.."
	$(CC) -o $@ $(objs)
%.o: %.c
	@echo "Compiling sources.."
	$(CC) -c $(sources)
%.d: %.c
	@echo -n  "Creating dependancies.."
	@echo $@
	@set -e; rm -f $@; \
	$(CC) -MM $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,    \($*\)\.o[ :]*     ,    \1.o $@ :   ,   g' < $@.$$$$ > $@; \
	rm -f $@.$$$$
clean:
	@rm -f main *.o *.d*
	@echo "Clean done.."

-include $(sources:.c=.d)

