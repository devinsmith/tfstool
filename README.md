tfstool
=======

What is this?
-------------

This is a tool that can interact with Microsoft TFS in a similar way that
tf.exe can on Windows. In fact, if you are using Microsoft Windows you should
stick with that tool. We hope to provide a subset of that functionality on
non-Microsoft systems.

Building
--------

```
cd src; make.
```

Configuration
-------------

Your TFS server, username, and password should be placed in a ~/.tfsrc
file.

```ini
; Configuration for tfstool

[tfs]
base_url=https://my-tfs-server.com/tfs/Organization
default_project=MyProject
username=DOMAIN\username
password=MySecurePassword
```

License
-------

This tool is licensed under the MIT license.
