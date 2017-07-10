****************************************
:mod:`network` --- network configuration
****************************************

.. module:: network
   :synopsis: network configuration

This module provides network drivers and routing configuration. Network
drivers for specific hardware are available within this module and are
used to configure a hardware network interface. Configured interfaces
are then available for use via the :mod:`socket` module. To use this module
the network build of firmware must be installed.

For example::

    # configure a specific network interface
    # see below for examples of specific drivers
    import network
    nic = network.Driver(...)
    print(nic.ifconfig())

    # now use socket as usual
    import socket
    addr = socket.getaddrinfo('micropython.org', 80)[0][-1]
    s = socket.socket()
    s.connect(addr)
    s.send(b'GET / HTTP/1.1\r\nHost: micropython.org\r\n\r\n')
    data = s.recv(1000)
    s.close()

.. only:: port_wipy

    .. _network.Server:

    class Server
    ============

    The ``Server`` class controls the behavior and the configuration of the FTP and telnet
    services running on the WiPy. Any changes performed using this class' methods will
    affect both.

    Example::

        import network
        server = network.Server()
        server.deinit() # disable the server
        # enable the server again with new settings
        server.init(login=('user', 'password'), timeout=600)

    Constructors
    ------------

    .. class:: network.Server(id, ...)

       Create a server instance, see ``init`` for parameters of initialization.

    Methods
    -------

    .. method:: server.init(\*, login=('micro', 'python'), timeout=300)

       Init (and effectively start the server). Optionally a new ``user``, ``password``
       and ``timeout`` (in seconds) can be passed.

    .. method:: server.deinit()

       Stop the server

    .. method:: server.timeout([timeout_in_seconds])

       Get or set the server timeout.

    .. method:: server.isrunning()

       Returns ``True`` if the server is running, ``False`` otherwise.

.. only:: port_pyboard

    class CC3K
    ==========

    This class provides a driver for CC3000 wifi modules. Example usage::

        import network
        nic = network.CC3K(pyb.SPI(2), pyb.Pin.board.Y5, pyb.Pin.board.Y4, pyb.Pin.board.Y3)
        nic.connect('your-ssid', 'your-password')
        while not nic.isconnected():
            pyb.delay(50)
        print(nic.ifconfig())

        # now use socket as usual
        ...

    For this example to work the CC3000 module must have the following connections:

        - MOSI connected to Y8
        - MISO connected to Y7
        - CLK connected to Y6
        - CS connected to Y5
        - VBEN connected to Y4
        - IRQ connected to Y3

    It is possible to use other SPI busses and other pins for CS, VBEN and IRQ.

    Constructors
    ------------

    .. class:: CC3K(spi, pin_cs, pin_en, pin_irq)

       Create a CC3K driver object, initialize the CC3000 module using the given SPI bus
       and pins, and return the CC3K object.

       Arguments are:

         - ``spi`` is an :ref:`SPI object <pyb.SPI>` which is the SPI bus that the CC3000 is
           connected to (the MOSI, MISO and CLK pins).
         - ``pin_cs`` is a :ref:`Pin object <pyb.Pin>` which is connected to the CC3000 CS pin.
         - ``pin_en`` is a :ref:`Pin object <pyb.Pin>` which is connected to the CC3000 VBEN pin.
         - ``pin_irq`` is a :ref:`Pin object <pyb.Pin>` which is connected to the CC3000 IRQ pin.

       All of these objects will be initialized by the driver, so there is no need to
       initialize them yourself. For example, you can use::

         nic = network.CC3K(pyb.SPI(2), pyb.Pin.board.Y5, pyb.Pin.board.Y4, pyb.Pin.board.Y3)

    Methods
    -------

    .. method:: cc3k.connect(ssid, key=None, \*, security=WPA2, bssid=None)

       Connect to a wifi access point using the given SSID, and other security
       parameters.

    .. method:: cc3k.disconnect()

       Disconnect from the wifi access point.

    .. method:: cc3k.isconnected()

       Returns True if connected to a wifi access point and has a valid IP address,
       False otherwise.

    .. method:: cc3k.ifconfig()

       Returns a 7-tuple with (ip, subnet mask, gateway, DNS server, DHCP server,
       MAC address, SSID).

    .. method:: cc3k.patch_version()

       Return the version of the patch program (firmware) on the CC3000.

    .. method:: cc3k.patch_program('pgm')

       Upload the current firmware to the CC3000. You must pass 'pgm' as the first
       argument in order for the upload to proceed.

    Constants
    ---------

    .. data:: CC3K.WEP
    .. data:: CC3K.WPA
    .. data:: CC3K.WPA2

       security type to use

    class WIZNET5K
    ==============

    This class allows you to control WIZnet5x00 Ethernet adapters based on
    the W5200 and W5500 chipsets (only W5200 tested).

    Example usage::

        import network
        nic = network.WIZNET5K(pyb.SPI(1), pyb.Pin.board.X5, pyb.Pin.board.X4)
        print(nic.ifconfig())

        # now use socket as usual
        ...

    For this example to work the WIZnet5x00 module must have the following connections:

        - MOSI connected to X8
        - MISO connected to X7
        - SCLK connected to X6
        - nSS connected to X5
        - nRESET connected to X4

    It is possible to use other SPI busses and other pins for nSS and nRESET.

    Constructors
    ------------

    .. class:: WIZNET5K(spi, pin_cs, pin_rst)

       Create a WIZNET5K driver object, initialize the WIZnet5x00 module using the given
       SPI bus and pins, and return the WIZNET5K object.

       Arguments are:

         - ``spi`` is an :ref:`SPI object <pyb.SPI>` which is the SPI bus that the WIZnet5x00 is
           connected to (the MOSI, MISO and SCLK pins).
         - ``pin_cs`` is a :ref:`Pin object <pyb.Pin>` which is connected to the WIZnet5x00 nSS pin.
         - ``pin_rst`` is a :ref:`Pin object <pyb.Pin>` which is connected to the WIZnet5x00 nRESET pin.

       All of these objects will be initialized by the driver, so there is no need to
       initialize them yourself. For example, you can use::

         nic = network.WIZNET5K(pyb.SPI(1), pyb.Pin.board.X5, pyb.Pin.board.X4)

    Methods
    -------

    .. method:: wiznet5k.ifconfig([(ip, subnet, gateway, dns)])

       Get/set IP address, subnet mask, gateway and DNS.

       When called with no arguments, this method returns a 4-tuple with the above information.

       To set the above values, pass a 4-tuple with the required information. For example::

        nic.ifconfig(('192.168.0.4', '255.255.255.0', '192.168.0.1', '8.8.8.8'))

    .. method:: wiznet5k.regs()

       Dump the WIZnet5x00 registers. Useful for debugging.

.. _network.WLAN:

.. only:: port_esp8266

    Functions
    =========

    .. function:: phy_mode([mode])

        Get or set the PHY mode.

        If the ``mode`` parameter is provided, sets the mode to its value. If
        the function is called without parameters, returns the current mode.

        The possible modes are defined as constants:
            * ``MODE_11B`` -- IEEE 802.11b,
            * ``MODE_11G`` -- IEEE 802.11g,
            * ``MODE_11N`` -- IEEE 802.11n.

    class WLAN
    ==========

    This class provides a driver for WiFi network processor in the ESP8266. Example usage::

        import network
        # enable station interface and connect to WiFi access point
        nic = network.WLAN(network.STA_IF)
        nic.active(True)
        nic.connect('your-ssid', 'your-password')
        # now use sockets as usual

    Constructors
    ------------
    .. class:: WLAN(interface_id)

    Create a WLAN network interface object. Supported interfaces are
    ``network.STA_IF`` (station aka client, connects to upstream WiFi access
    points) and ``network.AP_IF`` (access point, allows other WiFi clients to
    connect). Availability of the methods below depends on interface type.
    For example, only STA interface may ``connect()`` to an access point.

    Methods
    -------

    .. method:: wlan.active([is_active])

        Activate ("up") or deactivate ("down") network interface, if boolean
        argument is passed. Otherwise, query current state if no argument is
        provided. Most other methods require active interface.

    .. method:: wlan.connect(ssid, password)

        Connect to the specified wireless network, using the specified password.

    .. method:: wlan.disconnect()

        Disconnect from the currently connected wireless network.

    .. method:: wlan.scan()

        Scan for the available wireless networks.

        Scanning is only possible on STA interface. Returns list of tuples with
        the information about WiFi access points:

            (ssid, bssid, channel, RSSI, authmode, hidden)

        `bssid` is hardware address of an access point, in binary form, returned as
        bytes object. You can use ``ubinascii.hexlify()`` to convert it to ASCII form.

        There are five values for authmode:

            * 0 -- open
            * 1 -- WEP
            * 2 -- WPA-PSK
            * 3 -- WPA2-PSK
            * 4 -- WPA/WPA2-PSK

        and two for hidden:

            * 0 -- visible
            * 1 -- hidden

    .. method:: wlan.status()

        Return the current status of the wireless connection.

        The possible statuses are defined as constants:

            * ``STAT_IDLE`` -- no connection and no activity,
            * ``STAT_CONNECTING`` -- connecting in progress,
            * ``STAT_WRONG_PASSWORD`` -- failed due to incorrect password,
            * ``STAT_NO_AP_FOUND`` -- failed because no access point replied,
            * ``STAT_CONNECT_FAIL`` -- failed due to other problems,
            * ``STAT_GOT_IP`` -- connection successful.

    .. method:: wlan.isconnected()

        In case of STA mode, returns ``True`` if connected to a wifi access
        point and has a valid IP address. In AP mode returns ``True`` when a
        station is connected. Returns ``False`` otherwise.

    .. method:: wlan.ifconfig([(ip, subnet, gateway, dns)])

       Get/set IP-level network interface parameters: IP address, subnet mask,
       gateway and DNS server. When called with no arguments, this method returns
       a 4-tuple with the above information. To set the above values, pass a
       4-tuple with the required information. For example::

        nic.ifconfig(('192.168.0.4', '255.255.255.0', '192.168.0.1', '8.8.8.8'))

    .. method:: wlan.config('param')
    .. method:: wlan.config(param=value, ...)

       Get or set general network interface parameters. These methods allow to work
       with additional parameters beyond standard IP configuration (as dealt with by
       ``wlan.ifconfig()``). These include network-specific and hardware-specific
       parameters. For setting parameters, keyword argument syntax should be used,
       multiple parameters can be set at once. For querying, parameters name should
       be quoted as a string, and only one parameter can be queries at time::

        # Set WiFi access point name (formally known as ESSID) and WiFi channel
        ap.config(essid='My AP', channel=11)
        # Queey params one by one
        print(ap.config('essid'))
        print(ap.config('channel'))

       Following are commonly supported parameters (availability of a specific parameter
       depends on network technology type, driver, and MicroPython port).

       =========  ===========
       Parameter  Description
       =========  ===========
       mac        MAC address (bytes)
       essid      WiFi access point name (string)
       channel    WiFi channel (integer)
       hidden     Whether ESSID is hidden (boolean)
       authmode   Authentication mode supported (enumeration, see module constants)
       password   Access password (string)
       =========  ===========

.. only:: port_wipy

    class WLAN
    ==========

    This class provides a driver for the WiFi network processor in the WiPy. Example usage::

        import network
        import time
        # setup as a station
        wlan = network.WLAN(mode=WLAN.STA)
        wlan.connect('your-ssid', auth=(WLAN.WPA2, 'your-key'))
        while not wlan.isconnected():
            time.sleep_ms(50)
        print(wlan.ifconfig())

        # now use socket as usual
        ...

    Constructors
    ------------

    .. class:: WLAN(id=0, ...)

       Create a WLAN object, and optionally configure it. See ``init`` for params of configuration.

    .. note::

       The ``WLAN`` constructor is special in the sense that if no arguments besides the id are given,
       it will return the already existing ``WLAN`` instance without re-configuring it. This is
       because ``WLAN`` is a system feature of the WiPy. If the already existing instance is not
       initialized it will do the same as the other constructors an will initialize it with default
       values.

    Methods
    -------

    .. method:: wlan.init(mode, \*, ssid, auth, channel, antenna)

       Set or get the WiFi network processor configuration.

       Arguments are:

         - ``mode`` can be either ``WLAN.STA`` or ``WLAN.AP``.
         - ``ssid`` is a string with the ssid name. Only needed when mode is ``WLAN.AP``.
         - ``auth`` is a tuple with (sec, key). Security can be ``None``, ``WLAN.WEP``,
           ``WLAN.WPA`` or ``WLAN.WPA2``. The key is a string with the network password.
           If ``sec`` is ``WLAN.WEP`` the key must be a string representing hexadecimal
           values (e.g. 'ABC1DE45BF'). Only needed when mode is ``WLAN.AP``.
         - ``channel`` a number in the range 1-11. Only needed when mode is ``WLAN.AP``.
         - ``antenna`` selects between the internal and the external antenna. Can be either
           ``WLAN.INT_ANT`` or ``WLAN.EXT_ANT``.

       For example, you can do::

          # create and configure as an access point
          wlan.init(mode=WLAN.AP, ssid='wipy-wlan', auth=(WLAN.WPA2,'www.wipy.io'), channel=7, antenna=WLAN.INT_ANT)

       or::

          # configure as an station
          wlan.init(mode=WLAN.STA)

    .. method:: wlan.connect(ssid, \*, auth=None, bssid=None, timeout=None)

       Connect to a wifi access point using the given SSID, and other security
       parameters.

          - ``auth`` is a tuple with (sec, key). Security can be ``None``, ``WLAN.WEP``,
            ``WLAN.WPA`` or ``WLAN.WPA2``. The key is a string with the network password.
            If ``sec`` is ``WLAN.WEP`` the key must be a string representing hexadecimal
            values (e.g. 'ABC1DE45BF').
          - ``bssid`` is the MAC address of the AP to connect to. Useful when there are several
            APs with the same ssid.
          - ``timeout`` is the maximum time in milliseconds to wait for the connection to succeed.

    .. method:: wlan.scan()

       Performs a network scan and returns a list of named tuples with (ssid, bssid, sec, channel, rssi).
       Note that channel is always ``None`` since this info is not provided by the WiPy.

    .. method:: wlan.disconnect()

       Disconnect from the wifi access point.

    .. method:: wlan.isconnected()

       In case of STA mode, returns ``True`` if connected to a wifi access point and has a valid IP address.
       In AP mode returns ``True`` when a station is connected, ``False`` otherwise.

    .. method:: wlan.ifconfig(if_id=0, config=['dhcp' or configtuple])

       With no parameters given eturns a 4-tuple of ``(ip, subnet_mask, gateway, DNS_server)``.

       if ``'dhcp'`` is passed as a parameter then the DHCP client is enabled and the IP params
       are negotiated with the AP.

       If the 4-tuple config is given then a static IP is configured. For instance::

          wlan.ifconfig(config=('192.168.0.4', '255.255.255.0', '192.168.0.1', '8.8.8.8'))

    .. method:: wlan.mode([mode])

       Get or set the WLAN mode.

    .. method:: wlan.ssid([ssid])

       Get or set the SSID when in AP mode.

    .. method:: wlan.auth([auth])

       Get or set the authentication type when in AP mode.

    .. method:: wlan.channel([channel])

       Get or set the channel (only applicable in AP mode).

    .. method:: wlan.antenna([antenna])

       Get or set the antenna type (external or internal).

    .. method:: wlan.mac([mac_addr])

       Get or set a 6-byte long bytes object with the MAC address.

    .. method:: wlan.irq(\*, handler, wake)

        Create a callback to be triggered when a WLAN event occurs during ``machine.SLEEP``
        mode. Events are triggered by socket activity or by WLAN connection/disconnection.

            - ``handler`` is the function that gets called when the irq is triggered.
            - ``wake`` must be ``machine.SLEEP``.

        Returns an irq object.

    Constants
    ---------

    .. data:: WLAN.STA
    .. data:: WLAN.AP

       selects the WLAN mode

    .. data:: WLAN.WEP
    .. data:: WLAN.WPA
    .. data:: WLAN.WPA2

       selects the network security

    .. data:: WLAN.INT_ANT
    .. data:: WLAN.EXT_ANT

       selects the antenna type

.. only:: port_openmvcam

    class WINC -- wifi shield driver
    ================================

    The ``WINC`` class is used for controlling the wifi shield.

    Example usage::

        import network

        wlan = network.WINC()
        wlan.connect("SSID", "KEY")

        wlan.ifconfig()

    Constructors
    ------------

    .. class:: WINC(mode=MODE_STATION)

       Creates a winc driver object and connects to the wifi shield which uses
       I/O pins P0, P1, P2, P3, P6, P7, and P8.

       ``mode`` controls the mode the WINC module works in:

         * network.WINC.MODE_STATION

           The module connects to an access point as a client. This is the default mode.

         * network.WINC.MODE_AP

           The module will create an AP (Access Point) and
           accept connections from a client.

           Note1: The start_ap() function must be called after setting AP mode to configure the AP.

           Note2: The WINC1500 has some limitations in its AP implementation:

             * Only one client can connect at a time.
             * Only OPEN or WEP security are supported.
             * There's a bug in the FW, when the client disconnects any bound sockets are lost (they just stop working). As a workaround, set a timeout for the server socket to force it to raise an exception and then reopen it (See the example script).

         * network.WINC.MODE_P2P

           Enable Peer-to-Peer mode, also known as WiFiDirect. This mode is similar to AP, it allows two devices to connect and exchange data directly.
           Note: This mode is Not implemented Yet.

         * network.WINC.MODE_FIRMWARE:

           This mode enables WiFi module firmware update.
           Note: Do NOT use unless you know what you're doing, modules are shipped with the latest FW update there's No need to update the FW.

    Methods
    -------

    .. method:: winc.connect(ssid, key=None, security=WPA_PSK)

       Connect to a wifi network with ssid ``ssid`` using key ``key`` with
       security ``security``.

       After connecting to the network use the ``usocket`` module to open TCP/UDP
       ports to send and receive data.

       .. note::

          This function takes a little while to return.

    .. method:: winc.start_ap(SSID, key=None, security=OPEN, channel=1)

       When running in AP mode this function must be called after creating
       a WINC object to configure and start the AP .

         * SSID: The AP SSID (must be set)
         * Key: The AP encryption key. A Key is required only if security is WEP.
         * security: AP security mode. (Only network.WINC.OPEN or network.WINC.WEP are supported).
         * channel: WiFi channel, change this if you have another AP running at the same channel.

    .. method:: winc.disconnect()

       Disconnect from the wifi network.

    .. method:: winc.isconnected()

       Returns True if connected to an access point and an IP address has been
       obtained.

    .. method:: winc.ifconfig()

       Returns a tuple containing:

          * [0]: RSSI - received signal strength indicator (int)
          * [1]: Authorization Type (see constants)
          * [2]: Set Service Identifier String (SSID)
          * [3]: MAC Address String (XX:XX:XX:XX:XX:XX) (BSSID)
          * [4]: IP Address String (XXX.XXX.XXX.XXX)

       While connected to the network.

    .. method:: winc.scan()

       Returns a list of tuples containing:

          * [0]: Channel Number (int)
          * [1]: RSSI - received signal strength indicator (int)
          * [2]: Authorization Type (see constants)
          * [3]: MAC Address String (XX:XX:XX:XX:XX:XX) (BSSID)
          * [4]: Set Service Identifier String (SSID)

       You don't need to be connected to call this.

    .. method:: winc.rssi()

       Returns the received signal strength indicator (int) of the currently
       connected network.

    .. method:: winc.fw_version()

       Returns a tuple containing the wifi shield firmware version number.

          * [0]: Firmware Major Version Number (int)
          * [1]: Firmware Minor Version Number (int)
          * [2]: Firmware Patch Version Number (int)
          * [3]: Driver Major Version Number (int)
          * [4]: Driver Minor Version Number (int)
          * [5]: Driver Patch Version Number (int)
          * [6]: Hardware Revision Number - Chip ID (int)

    .. method:: winc.fw_dump()

       Dumps the wifi shield firmware to a binary at "/firmware/fw_dump.bin"

    .. method:: winc.fw_update()

       Programs the wifi shield with binary image found at
       "/firmware/m2m_aio_3a0.bin".

    .. method:: winc.connected_sta()

       This method returns the connected client IP.

    .. method:: winc.wait_for_sta(timeout)

       This function blocks and waits for a client to connect. If timeout is 0 this will block forever.

    Constants
    ---------

    .. data:: winc.OPEN

       For connecting to an open wifi network.

       .. note::

          Insecure.

    .. data:: winc.WEP

       For connecting to a WEP based password protected network.

       .. note::

          Insecure.

    .. data:: winc.WPA_PSK

       For connecting to a WPA/PSK based password protected network.

       .. note::

          For networks that need a password for all users.

    .. data:: winc.802_1X

       For connecting to a 802.1X based password protected network.

       .. note::

          For networks that need a separate password per user.
