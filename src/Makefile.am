AM_CPPFLAGS=@BOOST_CPPFLAGS@ -I $(top_srcdir)/asio-1.5.3/include/

bin_PROGRAMS=googleproxy
googleproxy_SOURCES=proxy.cpp sd-daemon.h

if HAVE_SYSTEMD
   googleproxy_SOURCES+=sd-daemon.c
endif
