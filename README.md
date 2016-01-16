C Web Server
============

This is a implementation of a basic HTTP 1.1 web server, in C using sockets
directly to create the connection and serve the files required by the system.

It does the minimum possible to serve files. The request headers are not parsed
at all. The response headers implemented are as fewer as possible. The server
will have a local directory as first argument, and the files from that directory
and subdirectories will we served.

Try running:

    make

And then:

    ./cwebserver .

Or:

    sudo ./cwebserver .

You should be able to navigate the folder, and in the web folder, there is a
simple html file with an image, to check that it works.
