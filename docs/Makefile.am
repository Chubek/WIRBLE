EXTRA_DIST = \
	Doxyfile.in \
	layout.xml

doxygen_output_dir = $(builddir)/doxygen
doxygen_html_dir = $(doxygen_output_dir)/html
doxygen_latex_dir = $(doxygen_output_dir)/latex
doxygen_man_dir = $(doxygen_output_dir)/man/man3wil
doxygen_stamp = $(builddir)/doxygen.stamp

all-local:
	@if test "x$(DOXYGEN)" != "x:"; then \
		$(MAKE) $(AM_MAKEFLAGS) docs; \
	fi

docs: $(doxygen_stamp)

$(doxygen_stamp): $(srcdir)/Doxyfile.in $(srcdir)/layout.xml $(top_srcdir)/README.md Doxyfile
	@if test "x$(DOXYGEN)" = "x:"; then \
		echo "Doxygen not found; skipping documentation generation."; \
		exit 0; \
	fi
	$(MKDIR_P) $(doxygen_output_dir)
	cd $(top_srcdir) && $(DOXYGEN) $(abs_builddir)/Doxyfile
	touch $@

pdf: docs
	@if test -f "$(doxygen_latex_dir)/Makefile"; then \
		$(MAKE) -C "$(doxygen_latex_dir)" pdf; \
	fi

clean-local:
	rm -rf "$(doxygen_output_dir)" "$(doxygen_stamp)" "$(builddir)/doxygen-warnings.log"

install-data-local:
if INSTALL_DOCS
	@if test "x$(DOXYGEN)" != "x:"; then \
		$(MAKE) $(AM_MAKEFLAGS) docs; \
		$(MKDIR_P) "$(DESTDIR)$(docdir)/html" "$(DESTDIR)$(docdir)/latex"; \
		if test -d "$(doxygen_html_dir)"; then cp -R "$(doxygen_html_dir)/." "$(DESTDIR)$(docdir)/html/"; fi; \
		if test -d "$(doxygen_latex_dir)"; then cp -R "$(doxygen_latex_dir)/." "$(DESTDIR)$(docdir)/latex/"; fi; \
		$(MAKE) $(AM_MAKEFLAGS) pdf; \
		if test -f "$(doxygen_latex_dir)/refman.pdf"; then \
			$(MKDIR_P) "$(DESTDIR)$(docdir)/pdf"; \
			$(INSTALL_DATA) "$(doxygen_latex_dir)/refman.pdf" "$(DESTDIR)$(docdir)/pdf/refman.pdf"; \
		fi; \
	fi
endif
if INSTALL_MANFILES
	@if test "x$(DOXYGEN)" != "x:"; then \
		$(MAKE) $(AM_MAKEFLAGS) docs; \
		$(MKDIR_P) "$(DESTDIR)$(mandir)/man3wil"; \
		if test -d "$(doxygen_man_dir)"; then cp -R "$(doxygen_man_dir)/." "$(DESTDIR)$(mandir)/man3wil/"; fi; \
	fi
endif
