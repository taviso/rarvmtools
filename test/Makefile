CPPFLAGS	= -I../stdlib
RARAS		= ../raras
RARLD		= ../rarld

%.ri: %.rs
	cpp $(CPPFLAGS) < $< > $@

%.ro: %.ri
	$(RARAS) -o $@ $<

%.rar: %.ro
	$(RARLD) $< > $@

all:   helloworld.rar crc32.rar bswap.rar mod.rar bitorder.rar vectormatch.rar \
       vectorrow.rar compensate.rar operands.rar fib.rar
	test "$$(unrar p -inul helloworld.rar)" == "Hello, World!"
	test "$$(unrar p -inul crc32.rar)" == "OK"
	test "$$(unrar p -inul bswap.rar)" == "OK"
	test "$$(unrar p -inul mod.rar)" == "OK"
	test "$$(unrar p -inul bitorder.rar)" == "OK"
	test "$$(unrar p -inul vectormatch.rar)" == "OK"
	test "$$(unrar p -inul compensate.rar)" == "OK"
	test "$$(unrar p -inul vectorrow.rar)" == "OK"
	test "$$(unrar p -inul operands.rar)" == "OK"
	test "$$(unrar p -inul fib.rar)" == "OK"
clean:
	rm -f *.ri *.ro *.rar
