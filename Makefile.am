SUBDIRS = src contrib/http-parser
dist_doc_DATA = README

PROGNAME=src/eenvoudig

test: $(PROGNAME)
	$(PROGNAME) -l localhost -p 45678 -b 5
