SUFFIXES=.ecpp .gif .jpg .css .js .ico
.ecpp.cpp:
	ecppc $(ECPPFLAGS) $(ECPPFLAGS_CPP) $<
.png.cpp:
	ecppc $(ECPPFLAGS) -b $(ECPPFLAGS_PNG) $<
.js.cpp:
	ecppc $(ECPPFLAGS) -b $(ECPPFLAGS_JS) $<
.css.cpp:
	ecppc $(ECPPFLAGS) -b $(ECPPFLAGS_CSS) $<

AM_CPPFLAGS=-I$(top_builddir)/include

bin_PROGRAMS = ZimReader

S = commonPrint_css.css main_css.css wikibits_js.js monobookde_css.css \
    user_css.css zimwp_css.css Wiki_png.png common_css.css headbg_jpg.jpg

ZimReader_SOURCES = zimcomp.ecpp skin.ecpp \
	search.ecpp searcharticles.ecpp searchresults.ecpp article.ecpp \
	tntnet_png.png random.ecpp notfound.ecpp number.ecpp \
	pager.ecpp browse.ecpp browsescreen.ecpp browseresults.ecpp \
	ajax_js.js redirect.ecpp main.cpp \
	$(S)

ZimReader_LDFLAGS = -lzim -ltntnet

EXTRA_DIST = global.ecpp
