CFLAGS	= -ggdb3 -O0 -march=native -std=gnu99 -Wall
LDFLAGS	= $(CFLAGS)
LDLIBS = -lz
RARAS	= ./raras
RARLD	= ./rarld

%.ri: %.rs
	cpp -Istdlib < $< > $@

%.ro: %.ri
	$(RARAS) -o $@ $<

%.rar: %.ro
	$(RARLD) $< > $@

all:   raras rarld sample.rar test
rarld: rarld.o bitbuffer.o
raras: parser.o raras.o bitbuffer.o
bitbuffer_test: bitbuffer_test.o bitbuffer.o

test: bitbuffer_test
	./bitbuffer_test
	make -C test all

clean:
	rm -f *.o raras rarld *_test *.ri *.ro *.rar
	make -C test clean
