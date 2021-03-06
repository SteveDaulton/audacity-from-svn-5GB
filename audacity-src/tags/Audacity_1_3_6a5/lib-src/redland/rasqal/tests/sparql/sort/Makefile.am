# -*- Mode: Makefile -*-
#
# Makefile.am - automake file for Rasqal SPARQL tests
#
# Copyright (C) 2005-2008, David Beckett http://www.dajobe.org/
# Copyright (C) 2005-2005, University of Bristol, UK http://www.bristol.ac.uk/
# 
# This package is Free Software and part of Redland http://librdf.org/
# 
# It is licensed under the following three licenses as alternatives:
#   1. GNU Lesser General Public License (LGPL) V2.1 or any newer version
#   2. GNU General Public License (GPL) V2 or any newer version
#   3. Apache License, V2.0 or any newer version
# 
# You may not use this file except in compliance with at least one of
# the above three licenses.
# 
# See LICENSE.html or LICENSE.txt at the top of this package for the
# complete terms and further detail along with the license texts for
# the licenses in COPYING.LIB, COPYING and LICENSE-2.0.txt respectively.
# 

SPARQL_MANIFEST_FILES= manifest.ttl

SPARQL_MODEL_FILES= \
data-sort-1.ttl data-sort-3.ttl data-sort-4.ttl data-sort-6.ttl	\
data-sort-7.ttl data-sort-8.ttl

SPARQL_TEST_FILES= \
query-sort-1.rq query-sort-2.rq query-sort-3.rq query-sort-4.rq	\
query-sort-5.rq query-sort-6.rq

# Test failures
EXPECTED_SPARQL_CORRECT_FAILURES=

SPARQL_RESULT_FILES= \
result-sort-1.rdf result-sort-2.rdf result-sort-3.rdf	\
result-sort-4.rdf result-sort-5.rdf result-sort-6.rdf	\
result-sort-7.rdf result-sort-8.rdf


EXTRA_DIST= \
$(SPARQL_MANIFEST_FILES) \
$(SPARQL_MODEL_FILES) \
$(SPARQL_TEST_FILES) \
$(SPARQL_RESULT_FILES)

build-roqet:
	@(cd $(top_builddir)/utils ; $(MAKE) roqet)

build-sparql-lexer-test:
	@(cd $(top_builddir)/src ; $(MAKE) sparql_lexer_test)

build-sparql-parser-test:
	@(cd $(top_builddir)/src ; $(MAKE) sparql_parser_test)


check-local: check-sparql-lexer check-sparql-parser \
check-sparql-correct

check-sparql-lexer: build-sparql-lexer-test
	@set +e; result=0; \
	$(ECHO) "Testing legal SPARQL lexing"; \
	for test in $(SPARQL_TEST_FILES); do \
	  $(ECHO) $(ECHO_N) "Checking $$test $(ECHO_C)"; \
	  $(top_builddir)/src/sparql_lexer_test $(srcdir)/$$test >/dev/null; \
	  status=$$?; \
	  if test $$status != 0 -a $$status != 2 ; then \
	    $(ECHO) FAILED returned status $$status; result=1; \
	  else \
	    $(ECHO) ok; \
	  fi; \
	done; \
	set -e; exit $$result

check-sparql-parser: build-sparql-parser-test
	@set +e; result=0; \
	$(ECHO) "Testing legal SPARQL parsing"; \
	for test in $(SPARQL_TEST_FILES); do \
	  $(ECHO) $(ECHO_N) "Checking $$test $(ECHO_C)"; \
	  $(top_builddir)/src/sparql_parser_test $(srcdir)/$$test >/dev/null; \
	  status=$$?; \
	  if test $$status != 0 -a $$status != 2 ; then \
	    $(ECHO) FAILED returned status $$status; result=1; \
	    fails="$$fails $$test"; \
	  else \
	    $(ECHO) ok; \
	  fi; \
	done; \
	set -e; exit $$result

check-sparql-correct: build-roqet $(srcdir)/../check-sparql
	@set +e; \
	expect=`echo $(EXPECTED_SPARQL_CORRECT_FAILURES) | wc -w`; \
	$(ECHO) "Testing SPARQL correct"; \
	PATH=$(top_builddir)/utils:$(top_builddir)@RAPPER_PATH@:$$PATH \
	  $(PERL) $(srcdir)/../check-sparql -s $(srcdir); \
	result=$$?; \
	if test $$result != 0 ; then \
	  echo "Expecting test failures: $(EXPECTED_SPARQL_CORRECT_FAILURES)"; \
	  if test $$result = $$expect ; then \
	    echo "OK: Ignoring $$expect expected test failures"; result=0; \
          else \
	    echo "FAILED: $$result failures does not match expected $$expect"; result=1; \
          fi; \
	fi; \
	set -e; exit $$result
