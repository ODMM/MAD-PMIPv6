## $Id: Makefile.am 1.3 03/12/12 14:03:21+02:00 anttit@tcs.hut.fi $
## Process this file with automake to produce Makefile.in

dist_man1_MANS = mad-pmip6d.man
dist_man5_MANS = mad-pmip6d.conf.man
##dist_man7_MANS = mipv6.man
BUILT_SOURCES = $(dist_man1_MANS) $(dist_man5_MANS)
##$(dist_man7_MANS)
CLEANFILES = $(dist_man1_MANS) $(dist_man5_MANS)
##$(dist_man7_MANS)
EXTRA_DIST = mad-pmip6d.tmpl mad-pmip6d.conf.tmpl
##mipv6.tmpl

%.man: %.tmpl
	$(SED) -e 's#%DATE%#@PACKAGE_DATE@#' \
               -e 's#%VERSION%#@PACKAGE_VERSION@#' \
               -e 's#%etc%#@sysconfdir@#' $< > $@
