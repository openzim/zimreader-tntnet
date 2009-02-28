#!/bin/sh

g++ -static -g -O2 -o TntReader zimcomp.o skin.o search.o searcharticles.o searchresults.o article.o tntnet_png.o random.o notfound.o number.o backgroundreader.o pager.o browse.o browsescreen.o browseresults.o ajax_js.o redirect.o main.o commonPrint_css.o main_css.o wikibits_js.o monobookde_css.o user_css.o zimwp_css.o Wiki_png.o common_css.o -lzim -ltntnet -lcxxtools -lpthread -lz -ldl -lbz2
