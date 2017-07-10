Software Setup
==============

Before you can start using your OpenMV Cam you'll need to download and install
`OpenMV IDE <https://openmv.io/pages/download/>`_.

Windows
-------

OpenMV IDE comes inside of an installer that will automatically install the IDE
along with drivers for the OpenMV Cam and the MicroPython pyboard. Just follow
the default installer prompts.

To launch OpenMV IDE just click on its shortcut in the Start Menu.

Mac
---

OpenMV IDE comes in a .dmg file. Simply open the .dmg file and then drag the
OpenMV IDE .app onto the Applications folder shortcut.

In addition to installing OpenMV IDE you'll also want to install device
firmware update (DFU) support so that you can recover your OpenMV Cam if you
encounter a problem with our boot-loader.

First you'll need to install a package manager. Either
`MacPorts <https://www.macports.org/install.php>`_ or
`HomeBrew <http://brew.sh/>`_. If you don't have a preference choose HomeBrew.

For MacPorts you'll need to execute the following commands::

    sudo port install libusb py-pip
    sudo pip install pyusb

For HomeBrew you'll need to execute the following commands::

    sudo brew install libusb python
    sudo pip install pyusb

In general, you have to install ``libusb``, ``python``, and ``pyusb``.

To launch OpenMV IDE just click on it's .app file in your Applications folder.

Linux
-----

OpenMV IDE comes inside of an installer .run file. Just do::

    chmod +x openmv-ide-linux-*.run
    ./openmv-ide-linux-*.run

In addition to installing OpenMV IDE you'll also want to install device
firmware update (DFU) support so that you can recover your OpenMV Cam if you
encounter a problem with our boot-loader.

Execute the following commands (or equivalent)::

    sudo apt-get install libusb-1.0 python-pip
    sudo pip install pyusb

In general, you have to install ``libusb``, ``python``, and ``pyusb``.

You'll also need to add yourself to the ``dialout`` group if you haven't
already before. Just do Do::

    sudo adduser <username> dialout

Where ``<username>`` is your user name and then logout and log back in again.

Next, you should copy the openmv udev rules to your udev settings directory.
To do this first navigate to the openmvide install directory which defaults to
``~/openmvide/`` or ``/opt/openmvide/`` if you installed OpenMV IDE as root.
Then do::

    sudo cp share/qtcreator/pydfu/50-openmv.rules /etc/udev/rules.d/50-openmv.rules
    sudo udevadm control --reload-rules

The udev rules will install a symlink for the OpenMV Cam in your ``/dev``
directory and allow OpenMV IDE to access your OpenMV Cam in normal mode and DFU
mode. In particular, the udev rules must be installed for OpenMV IDE to access
your OpenMV Cam in DFU mode. However, udev rules are not required for normal
mode if you've added yourself to the ``dialout`` group (above).

Finally, to launch OpenMV IDE just click on the ``bin/openmvide`` file. If
OpenMV IDE fails to launch you may need to add execution permission first. Do::

    chmod +x openmvide
    ./openmvide

If this doesn't work then use the ``openmvide.sh`` script instead. Do::

    chmod +x openmvide.sh
    ./openmvide.sh

In general, you'll need to run OpenMV IDE using the ``openmvide.sh`` script if
you've installed Qt on linux. The ``openmvide.sh`` script fixes linker path
issues before launching OpenMV IDE to make sure that it runs on any system.
