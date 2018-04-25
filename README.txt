							jxutil readme

The primary aim of this project is to create a JSON parser/generator
that has the following characteristics and supporting features:

1.	It should be able to compile on most systems.
2. 	Handling of incomplete input: the parser should be able to
	persist in an incomplete state, and be resumed once more input
	becomes available.
3.	Support for parsing on a separate thread to allow the main
	thread to continue buffering JSON may reduce the total parsing
	time.
4.	The parser should optionally generate a data-structure or issue
	callbacks, depending on user-preference.
5.	By default the parser/generator should be strict, but support
	for optional JSON type extensions that are cool and/or useful
	should exist, e.g. UTF-8 numeric constants, and Date.
6.	The library should support multiple data-sources, such as
	C-Strings, and UNIX file-descriptors (support for both blocking/
	non-blocking modes). And a low-level set of functions should be
	available to buffer input.

The secondary aim is to add some support for parsing/generating XML,
validating against a schema is not a priority, but could be done
once the primary goals are achieved.

- CJM
