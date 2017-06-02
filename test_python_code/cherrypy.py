#!/usr/bin/python
#
# CherryPy HelloWorld web server.

import cherrypy

class HelloWorld(object):
    def index(self):
        return "Hello World!"
    index.exposed = True

cherrypy.quickstart(HelloWorld())
