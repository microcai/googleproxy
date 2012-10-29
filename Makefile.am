
ACLOCAL_AMFLAGS = -I m4

SUBDIRS=src

EXTRA_DIST=googleproxy.socket googleproxy.service.in.in

systemdsystemunitdir=@systemdsystemunitdir@

if HAVE_SYSTEMD
systemdsystemunit_DATA = googleproxy.socket googleproxy.service 

googleproxy.service:googleproxy.service.in
	( \
		exec_prefix=${exec_prefix}; \
		pkgdatadir=${pkgdatadir}; \
		s=`cat $<`; \
		eval "echo \"$${s}\""; \
	) > $@

CLEANFILES = googleproxy.service
DISTCLEANFILES = googleproxy.service.in
endif

