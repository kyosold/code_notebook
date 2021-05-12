AUTOMAKE_OPTIONS=foreign

# [动态库]
lib_LTLIBRARIES=libct_mfile.la
libct_mfile_la_SOURCES=mfile.c
libct_mfile_la_CFLAGS=-g -I./

# 安装头文件到 {$prefix}/include/libct/mfile/ 下
mfileincludedir=$(includedir)/@PACKAGENAME@/mfile
mfileinclude_HEADERS=mfile.h queue.h

libdir=$(prefix)/lib64/@PACKAGENAME@/mfile

EXTRA_DIST=sample.c linux.mk

mfiledatadir=$(datarootdir)/@PACKAGENAME@/mfile
mfiledata_DATA=sample.c
