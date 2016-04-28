#!/usr/bin/env python3

import dbus

SERVER_NAME = 'org.tizen.lbs.Providers.FusedLocation'
SERVER_INTERFACE = 'org.tizen.lbs.Providers.FusedLocation.Server'
SERVER_PATH = '/org/tizen/lbs/Providers/FusedLocation'

class Client():
    def __init__(self):
        self.bus = dbus.SessionBus()
        self.fl = self.bus.get_object(SERVER_NAME, SERVER_PATH)
        self.registerClient = self.fl.get_dbus_method('registerClient', SERVER_INTERFACE)
        self.echo = self.fl.get_dbus_method('echo', SERVER_INTERFACE)
        self.setAccuracy = self.fl.get_dbus_method('setAccuracy', SERVER_INTERFACE)
        self.setDesiredInterval = self.fl.get_dbus_method('setDesiredInterval', SERVER_INTERFACE)
