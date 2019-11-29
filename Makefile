CFLAGS = -Wall -Werror -Isrc/

all: bin/jxutil.a

librel: rel/ bin/jxutil.a
	cp src/jx_util.h rel/jx_util.h
	cp src/jx_json.h rel/jx_json.h
	cp src/jx_value.h rel/jx_value.h
	cp bin/jxutil.a rel/jxutil.a

run_tests: jx_tests
	@./jx_tests

clean:
	@rm -rf bin/
	@rm -rf rel/
	@rm -rf tests/bin
	@rm -f jx_tests

bin/:
	@mkdir bin

rel/:
	@mkdir -p rel

tests/bin/:
	@mkdir tests/bin

bin/jxutil.a: bin/ bin/jx_util.o bin/jx_json.o bin/jx_value.o
	ar -rc bin/jxutil.a bin/jx_util.o bin/jx_json.o bin/jx_value.o

bin/jx_util.o: bin/ src/jx_util.c src/jx_util.h src/jx_value.h src/jx_json.h
	cc $(CFLAGS) -c src/jx_util.c -o bin/jx_util.o

bin/jx_json.o: bin/ src/jx_json.c src/jx_json.h src/jx_value.h
	cc $(CFLAGS) -c src/jx_json.c -o bin/jx_json.o

bin/jx_value.o: bin/ src/jx_value.c src/jx_value.h
	cc $(CFLAGS) -c src/jx_value.c -o bin/jx_value.o

tests/bin/jx_tests.o: tests/bin/ tests/jx_tests.c src/jx_util.h src/jx_json.h src/jx_value.h
	cc $(CFLAGS) -c tests/jx_tests.c -o tests/bin/jx_tests.o

tests/bin/jx_tests: tests/bin/jx_tests.o bin/jxutil.a
	cc $(CFLAGS) tests/bin/jx_tests.o bin/jxutil.a -o tests/bin/jx_tests

jx_tests: tests/bin/jx_tests
	ln -sf tests/bin/jx_tests jx_tests
