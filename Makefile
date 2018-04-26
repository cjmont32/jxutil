all: bin/jxutil.a

librel: rel/ bin/jxutil.a
	cp src/jx_util.h rel/jx_util.h
	cp src/jx_value.h rel/jx_value.h
	cp bin/jxutil.a rel/jxutil.a

run_tests: tests/bin/tests
	@tests/bin/tests

clean:
	@rm -rf bin/
	@rm -rf rel/
	@rm -rf tests/bin

bin/:
	@mkdir bin

rel/:
	@mkdir -p rel

tests/bin/:
	@mkdir tests/bin

bin/jxutil.a: bin/ bin/jx_util.o bin/jx_value.o
	ar -rc bin/jxutil.a bin/jx_util.o bin/jx_value.o

bin/jx_util.o: bin/ src/jx_util.c src/jx_util.h src/jx_value.h
	cc -Isrc/ -c src/jx_util.c -o bin/jx_util.o

bin/jx_value.o: bin/ src/jx_value.c src/jx_util.h src/jx_value.h
	cc -Isrc/ -c src/jx_value.c -o bin/jx_value.o

tests/bin/tests: tests/bin/ tests/tests.c bin/jxutil.a
	cc -Isrc/ tests/tests.c bin/jxutil.a -o tests/bin/tests