bin_PROGRAMS = eenvoudig
eenvoudig_SOURCES = main.c cmdline.c

main.c: cmdline.h

cmdline.c cmdline.h: cmdline.ggo
	gengetopt < $<
