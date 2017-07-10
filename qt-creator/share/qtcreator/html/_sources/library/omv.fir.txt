:mod:`fir` --- thermopile shield driver (fir == far infrared)
=============================================================

.. module:: fir
   :synopsis: thermopile shield driver (fir == far infrared)

The ``fir`` module is used for controlling the thermopile shield.

Example usage::

    import sensor, fir

    # Setup camera.
    sensor.reset()
    sensor.set_pixformat(sensor.RGB565)
    sensor.set_framesize(sensor.QVGA)
    sensor.skip_frames()
    fir.init()

    # Show image.
    while(True):
        img = sensor.snapshot()
        ta, ir, to_min, to_max = fir.read_ir()
        fir.draw_ir(image, ir)
        print("ambient temperature: %0.2f" % ta)
        print("min temperature seen: %0.2f" % to_min)
        print("max temperature seen: %0.2f" % to_max)

Functions
---------

.. function:: fir.init(type=1, refresh=64, resolution=18)

   Initializes an attached thermopile shield using I/O pins P4 and P5.

   ``type`` indicates the type of thermopile shield (for future use possibly):

      * 0: None
      * 1: thermopile shield

   ``refresh`` is the thermopile sensor power-of-2 refresh rate in Hz. Defaults
   to 64 Hz. Can be 1 Hz, 2 Hz, 4 Hz, 8 Hz, 16 Hz, 32 Hz, 64 Hz, 128 Hz, 256 Hz,
   or 512 Hz. Note that a higher refresh rate lowers the accuracy and vice-versa.

   ``resolution`` is the thermopile sensor measurement resolution. Defaults to
   18-bits. Can be 15-bits, 16-bits, 17-bits, or 18-bits. Note that a higher
   resolution lowers the maximum temperature range and vice-versa.

      * 15-bits -> Max of ~950C.
      * 16-bits -> Max of ~750C.
      * 17-bits -> Max of ~600C.
      * 18-bits -> Max of ~450C.

   .. note::

      ``type``, ``refresh``, and ``resolution`` are keyword arguments which must
      be explicitly invoked in the function call by writing ``type=``,
      ``refresh=``, and ``resolution=``.

.. function:: fir.deinit()

   Deinitializes the thermopile shield freeing up I/O pins.

.. function:: fir.width()

   Returns the width (horizontal resolution) of the thermopile shield.

      * None: 0 pixels.
      * thermopile shield: 16 pixels.

.. function:: fir.height()

   Returns the height (vertical resolution) of the thermopile shield.

      * None: 0 pixels.
      * thermopile shield: 4 pixels.

.. function:: fir.type()

   Returns the type of the thermopile shield (for future use possibly):

      * 0: None
      * 1: thermopile shield

.. function:: fir.read_ta()

   Returns the ambient temperature (i.e. sensor temperature).

   Example::

      ta = fir.read_ta()

   The value returned is a float that represents the temperature in Celsius.

.. function:: fir.read_ir()

   Returns a tuple containing the ambient temperature (i.e. sensor temperature),
   the temperature list (width * height), the minimum temperature seen, and
   the maximum temperature seen.

   Example::

      ta, ir, to_min, to_max = fir.read_ir()

   The values returned are floats that represent the temperature in Celsius.

   .. note::

      ``ir`` is a (width * height) list of floats.

.. function:: fir.draw_ta(image, ta, alpha=128, scale=[-17.7778, 37.7778])

   Draws the ambient temperature (``ta``) on the ``image`` using a rainbow
   table color conversion.

   ``alpha`` controls the transparency. 256 for an opaque overlay. 0 for none.

   ``scale`` controls the rainbow table color conversion. The first number is
   the minimum temperature cutoff and the second number is the max. Values
   closer to the min are blue and values closer to the max are red.

   The default ``scale`` of [-17.7778C, 37.7778C] corresponds to [0F, 100F].

   .. note::

      For best results look at really cold or hot objects.

   .. note::

      ``alpha`` and ``scale`` are keyword arguments which must be explicitly
      invoked in the function call by writing ``alpha=`` and ``scale=``.

.. function:: fir.draw_ta(image, ir, alpha=128, scale=[auto, auto])

   Draws the temperature list (``ir``) on the ``image`` using a rainbow
   table color conversion.

   ``alpha`` controls the transparency. 256 for an opaque overlay. 0 for none.

   ``scale`` controls the rainbow table color conversion. The first number is
   the minimum temperature cutoff and the second number is the max. Values
   closer to the min are blue and values closer to the max are red.

   The minimum and maximum values in the temperature list are used to scale
   the output ``image`` automatically unless explicitly overridden.

   .. note::

      For best results look at really cold or hot objects.

   .. note::

      ``alpha`` and ``scale`` are keyword arguments which must be explicitly
      invoked in the function call by writing ``alpha=`` and ``scale=``.
