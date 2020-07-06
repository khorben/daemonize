daemonize
=========

About daemonize
---------------

daemonize is a helper to turn regular commands into background services.

daemonize is distributed under the terms of the GNU GPL License (version 3).


Compiling daemonize
-------------------

daemonize is meant to be portable, and should not depend on any specific
component.

With GCC, this should be enough to compile and install daemonize:

    $ make install

To install (or package) daemonize in a different location:

    $ make PREFIX="/another/prefix" install

daemonize also supports `DESTDIR`, to be installed in a staging directory; for
instance:

    $ make DESTDIR="/staging/directory" PREFIX="/another/prefix" install


Contributing to daemonize
-------------------------

It is possible to re-generate the Makefiles for daemonize with `configure(1)`
from the DeforaOS configure project, found at
<https://www.defora.org/os/project/16/configure>. The procedure is then as
follows:

    $ configure
    $ make

Please refer to the documentation of DeforaOS configure for further
instructions.
