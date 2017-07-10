:mod:`sensor` --- camera sensor
===============================

.. module:: sensor
   :synopsis: camera sensor

The ``sensor`` module is used for taking pictures.

Example usage::

    import sensor

    # Setup camera.
    sensor.reset()
    sensor.set_pixformat(sensor.RGB565)
    sensor.set_framesize(sensor.QVGA)
    sensor.skip_frames()

    # Take pictures.
    while(True):
        sensor.snapshot()

Functions
---------

.. function:: sensor.reset()

   Initializes the camera sensor.

.. function:: sensor.snapshot(line_filter=None)

   Takes a picture using the camera and returns an ``image`` object.

   ``line_filter`` may be a python function callback used to process each line
   of pixels as they come in from the camera. For example::

      # This callback just copies the src to dst.
      # Note source is YUYV destination is 1BPP Grayscale
      def line_filter_call_back(src, dst):
        for i in range(len(src)):
          dst[i] = src[i>>1]
      sensor.snapshot(line_filter=line_filter_call_back)

      # This callback copies and thresholds src to dst.
      # Note source is YUYV destination is 1BPP Grayscale
      def line_filter_call_back_2(src, dst):
        for i in range(len(src)):
          dst[i] = if src[i>>1] > 128 then 0xFF or 0x00
      sensor.snapshot(line_filter=line_filter_call_back_2)

   .. note::

      The OpenMV Cam M4 is not fast enough to execute the line filter function
      on large images per line. Do not use.

   .. note::

      ``line_filter`` is keyword arguments which must be explicitly invoked in
      the function call by writing ``line_filter=``.

.. function:: sensor.skip_frames(n=10)

   Takes ``n`` number of snapshots to let the camera image stabilize after
   changing camera settings.

   .. note: You should call this function after changing camera settings.

.. function:: sensor.get_fb()

   (Get Frame Buffer) Returns the image object returned by a previous call of
   ``sensor.snapshot()``. If ``sensor.snapshot()`` had not been called before
   then ``None`` is returned.

.. function:: sensor.get_id()

   Returns the camera module ID.

      * sensor.OV2640: Old sensor module.
      * sensor.OV7725: New sensor module.

.. function:: sensor.set_pixformat(pixformat)

   Sets the pixel format for the camera module.

      * sensor.GRAYSCALE: 8-bits per pixel.
      * sensor.RGB565: 16-bits per pixel.

.. function:: sensor.set_framerate(rate)

   Sets the frame rate for the camera module.

   .. note:: Deprecated... do not use.

.. function:: sensor.set_framesize(framesize)

   Sets the frame size for the camera module.

      * sensor.B40X30: 160x120 (for use with ``image.find_displacement``)
      * sensor.B64X32: 160x120 (for use with ``image.find_displacement``)
      * sensor.B64X64: 160x120 (for use with ``image.find_displacement``)
      * sensor.QQVGA: 160x120
      * sensor.QQVGA2: 128x160 (for use with the lcd shield)
      * sensor.HQVGA: 220x160
      * sensor.QVGA: 320x120
      * sensor.VGA: 640x480 (only on the OpenMV Cam M7 in Grayscale)
      * sensor.SVGA: 800x600 (only in JPEG mode for the OV2640 sensor)
      * sensor.SXGA: 1280x1024 (only in JPEG mode for the OV2640 sensor)
      * sensor.UXGA: 1600x1200 (only in JPEG mode for the OV2640 sensor)
      * sensor.QQCIF: 88x72
      * sensor.QCIF: 176x144
      * sensor.CIF: 352x288

.. function:: sensor.set_windowing(roi)

   Sets the resolution of the camera to a sub resolution inside of the current
   resolution. For example, setting the resolution to sensor.VGA and then
   the windowing to (120, 140, 200, 200) sets sensor.snapshot() to capture
   the 200x200 center pixels of the VGA resolution outputted by the camera
   sensor. You can use windowing to get custom resolutions. Also, when using
   windowing on a larger resolution you effectively are digital zooming.

   ``roi`` is a rect tuple (x, y, w, h).

.. function:: sensor.set_gainceiling(gainceiling)

   Set the camera image gainceiling. 2, 4, 8, 16, 32, 64, or 128.

   .. note:: You should never need to call this function. Don't use.

.. function:: sensor.set_contrast(constrast)

   Set the camera image contrast. -3 to +3.

   .. note:: You should never need to call this function. Don't use.

.. function:: sensor.set_brightness(brightness)

   Set the camera image brightness. -3 to +3.

   .. note:: You should never need to call this function. Don't use.

.. function:: sensor.set_saturation(saturation)

   Set the camera image saturation. -3 to +3.

   .. note:: You should never need to call this function. Don't use.

.. function:: sensor.set_quality(quality)

   Set the camera image JPEG compression quality. 0 - 100.

   .. note:: Only for the OV2640 camera.

.. function:: sensor.set_colorbar(enable)

   Turns color bar mode on (True) or off (False). Defaults to off.

.. function:: sensor.set_auto_gain(enable, value=-1)

   ``enable`` Turns auto gain on (True) or off (False). Defaults to on.
   ``value`` Forced gain value. See the camera datasheet for more details.

   .. note:: You need to turn white balance off if you want to track colors.

   .. note::

      ``value`` is keyword arguments which must be explicitly invoked in the
      function call by writing ``value=``.

.. function:: sensor.set_auto_exposure(enable, value=-1)

   ``enable`` Turns auto exposure on (True) or off (False). Defaults to on.
   ``value`` Forced exposure value. See the camera datasheet for more details.

   .. note::

      ``value`` is keyword arguments which must be explicitly invoked in the
      function call by writing ``value=``.

.. function:: sensor.set_auto_whitebal(enable, value=(-1,-1,-1))

   ``enable`` Turns auto whitebal on (True) or off (False). Defaults to on.
   ``value`` Forced whitebal value. See the camera datasheet for more details.

   .. note:: You need to turn white balance off if you want to track colors.

   .. note::

      ``value`` is keyword arguments which must be explicitly invoked in the
      function call by writing ``value=``

.. function:: sensor.set_hmirror(enable)

   Turns horizontal mirror mode on (True) or off (False). Defaults to off.

.. function:: sensor.set_vflip(enable)

   Turns vertical flip mode on (True) or off (False). Defaults to off.

.. function:: sensor.set_special_effect(effect)

   Sets a camera image special effect:

      * sensor.NORMAL: Normal Image
      * sensor.NEGATIVE: Negative Image

   .. note:: Deprecated... do not use.

.. function:: sensor.set_lens_correction(enable, radi, coef)

   ``enable`` True to enable and False to disable (bool).
   ``radi`` integer radius of pixels to correct (int).
   ``coef`` power of correction (int).

.. function:: sensor.__write_reg(address, value)

   Write ``value`` (int) to camera register at ``address`` (int).

   .. note:: See the camera data sheet for register info.

.. function:: sensor.__read_reg(address)

   Read camera register at ``address`` (int).

   .. note:: See the camera data sheet for register info.

Constants
---------

.. data:: sensor.GRAYSCALE

   GRAYSCALE pixel format (Y from YUV422). Each pixel is 8-bits, 1-byte.

   All of our computer vision algorithms run faster on grayscale images than
   RGB565 images.

.. data:: sensor.RGB565

   RGB565 pixel format. Each pixel is 16-bits, 2-bytes. 5-bits are used for red,
   6-bits are used for green, and 5-bits are used for blue.

   All of our computer vision algorithms run slower on RGB565 images than
   grayscale images.

.. data:: sensor.JPEG

   JPEG mode. Only works for the OV2640 camera.

.. data:: sensor.YUV422

   Deprecated... do not use.

.. data:: sensor.OV9650:

   ``sensor.get_id()`` returns this for the OV9650 camera.

.. data:: sensor.OV2640:

   ``sensor.get_id()`` returns this for the OV2640 camera.

.. data:: sensor.OV7725:

   ``sensor.get_id()`` returns this for the OV7725 camera.

.. data:: sensor.B40X30

   40x30 resolution for the camera sensor (for use with ``image.find_displacement``).

.. data:: sensor.B64X32

   64x32 resolution for the camera sensor (for use with ``image.find_displacement``).

.. data:: sensor.B64X64

   64x64 resolution for the camera sensor (for use with ``image.find_displacement``).

.. data:: sensor.QQVGA

   160x120 resolution for the camera sensor.

.. data:: sensor.QQVGA2

   128x160 resolution for the camera sensor (for use with the lcd shield).

.. data:: sensor.QVGA

   320x120 resolution for the camera sensor.

.. data:: sensor.HQVGA

   220x160 resolution for the camera sensor.

.. data:: sensor.VGA

   640x480 resolution for the camera sensor.
   Only works for the OV2640 camera or the OpenMV Cam M7.

.. data:: sensor.QQCIF

   88x72 resolution for the camera sensor.

.. data:: sensor.QCIF

   176x144 resolution for the camera sensor.

.. data:: sensor.CIF

   352x288 resolution for the camera sensor.

.. data:: sensor.SVGA

   800x600 resolution for the camera sensor.
   Only works for the OV2640 camera.

.. data:: sensor.SXGA

   1280x1024 resolution for the camera sensor.
   Only works for the OV2640 camera.

.. data:: sensor.UXGA

   1600x1200 resolution for the camera sensor.
   Only works for the OV2640 camera.

.. data:: sensor.NORMAL

   Set the special effect filter to normal.

.. data:: sensor.NEGATIVE

   Set the special effect filter to negative.
