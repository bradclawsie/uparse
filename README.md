uparse
======

uparse is a c parser for common urls. This library does not support ipv6 addresses in 
urls. This library does not support urls with user authentication information ('@')
as these don't seem to be in use anymore. This library does not support non-ascii
urls (ut8 etc). This library does not strictly RFC 3986 compliant.

What this library does provide is parsing and basic error detection for most urls
you might encounter, with paths and query key/value pairs parsed into structures
you can inspect. Since the code is smaller than fully compliant libraries like
uriparser (http://uriparser.sourceforge.net), it may be useful as a learning tool.

The code was developed on FreeBSD using clang34 and not tested on any other platforms.

See the test programs for sample use. 


