AM_CPPFLAGS=@BOOST_CPPFLAGS@

bin_PROGRAMS=googleproxy
googleproxy_SOURCES=proxy.cpp sd-daemon.h

if HAVE_SYSTEMD
   googleproxy_SOURCES+=sd-daemon.c
endif
