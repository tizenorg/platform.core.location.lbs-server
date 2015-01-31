libexec_PROGRAMS = lbs-server

lbs_server_SOURCES =	lbs_server.c \
			server.c \
			data_connection.c \
			nmea_logger.c \
			gps_plugin_module.c \
			last_position.c \
			setting.c \
			dump_log.c \
			nps_plugin_module.c

lbs_server_CFLAGS =	-Wall -fPIC -fvisibility=hidden \
			-DEXPORT_API="__attribute__((visibility(\"default\")))" \
			-I$(srcdir)/include \
			$(PROVIDERS_CFLAGS)

lbs_server_LDFLAGS =	-Wl,--hash-style=both -Wl,--as-needed -Wl,-rpath=/usr/lib

lbs_server_DEPENDENCIES =

lbs_server_LDADD =	$(lbs_server_DEPENDENCIES) \
			$(PROVIDERS_LIBS) \
			-ldl \
			-lm	\
			$(NPS_LIBS)

lbs_server_plugin_hdr_intfdir = $(includedir)/lbs-server-plugin


lbs_server_plugin_hdr_intf_HEADERS =	\
			include/gps_plugin_intf.h \
			include/gps_plugin_data_types.h \
			include/gps_plugin_extra_data_types.h \
			include/nps_plugin_intf.h

PCFILES = lbs-server-plugin.pc
pkgconfigdir   = $(libdir)/pkgconfig
pkgconfig_DATA = $(PCFILES)

providersdir = $(datadir)/lbs
providers_DATA = lbs-server.provider

servicedir = $(DBUS_SERVICES_DIR)
service_in_files = org.tizen.lbs.Providers.LbsServer.service.in
service_DATA = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files) Makefile
	@sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@


$(conf_DATA):	$(conf_in_files)
		cp $< $@

EXTRA_DIST =	$(PCFILES).in \
		$(service_in_files) \
		$(providers_DATA) \
		$(conf_in_files)

DISTCLEANFILES =	$(service_DATA) \
			$(conf_DATA)

LBS_SERVER = lbs-server
bootdir = /etc/rc.d/init.d
boot_SCRIPTS = script/$(LBS_SERVER)

RC5_SCRIPT = $(DESTDIR)/etc/rc.d/rc5.d/S90lbs-server

install-exec-hook:
	mkdir -p $(DESTDIR)/etc/rc.d/rc5.d
	ln -sf ../init.d/$(LBS_SERVER) $(RC3_SCRIPT)
	ln -sf ../init.d/$(LBS_SERVER) $(RC5_SCRIPT)

uninstall-hook:
	rm -f $(RC3_SCRIPT) $(RC5_SCRIPT)

