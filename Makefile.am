SUBDIRS = . src test

if ENABLE_BENCHMARK
SUBDIRS += benchmark
endif

dist_doc_DATA = LICENSE README.md

ACLOCAL_AMFLAGS = -I m4

pkgconfigdir = @libdir@/pkgconfig
pkgconfig_DATA = @PACKAGE@.pc

.PHONY: lint

lint:
	cpplint --linelength=120 \
		--recursive src/ \
		--recursive test/ \
		--recursive benchmark/
