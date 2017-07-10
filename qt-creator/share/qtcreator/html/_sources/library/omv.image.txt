:mod:`image` --- machine vision
===============================

.. module:: image
   :synopsis: machine vision

The ``image`` module is used for machine vision.

Functions
---------

.. function:: image.rgb_to_lab(rgb_tuple)

   Returns the LAB tuple (l, a, b) for the RGB888 ``rgb_tuple`` (r, g, b).

   .. note::

      RGB888 means 8-bits (0-255) for red, green, and blue. For LAB, L
      goes from 0-100 and a/b go from -128 to 127.

.. function:: image.lab_to_rgb(lab_tuple)

   Returns the RGB888 tuple (r, g, b) for the LAB ``lab_tuple`` (l, a, b).

   .. note::

      RGB888 means 8-bits (0-255) for red, green, and blue. For LAB, L
      goes from 0-100 and a/b go from -128 to 127.

.. function:: image.rgb_to_grayscale(rgb_tuple)

   Returns the grayscale value for the RGB888 ``rgb_tuple`` (r, g, b).

   .. note::

      RGB888 means 8-bits (0-255) for red, green, and blue. The grayscale
      values goes between 0-255.

.. function:: image.grayscale_to_rgb(g_value)

   Returns the RGB888 tuple (r, a, b) for the grayscale ``g_value``.

   .. note::

       RGB888 means 8-bits (0-255) for red, green, and blue. The grayscale
       values goes between 0-255.

.. function:: image.load_decriptor(path)

   Loads a descriptor object from disk.

   ``path`` is the path to the descriptor file to load.

.. function:: image.save_descriptor(path, descriptor)

   Saves the descriptor object ``descriptor`` to disk.

   ``path`` is the path to the descriptor file to save.

.. function:: image.match_descriptor(descritor0, descriptor1, threshold=70, filter_outliers=False)

   For LBP descriptors this function returns an integer representing the
   difference between the two descriptors. You may then threshold/compare this
   distance metric as necessary. The distance is a measure of similarity. The
   closer it is to zero the better the LBP keypoint match.

   For ORB descriptors this function returns a tuple containing the following
   values:

       * [0] - X Centroid (int)
       * [1] - Y Centroid (int)
       * [2] - Bounding Box X (int)
       * [3] - Bounding Box Y (int)
       * [4] - Bounding Box W (int)
       * [5] - Bounding Box H (int)
       * [6] - Number of keypoints matched (int)
       * [7] - Estimated angle of rotation between keypoints in degrees.

   ``threshold`` is used for ORB keypoints to filter ambiguous matches. A lower
   ``threshold`` value tightens the keypoint matching algorithm. ``threshold``
   may be between 0-100 (int). Defaults to 70.

   ``filter_outliers`` is used for ORB keypoints to filter out outlier
   keypoints allow you to raise the ``threshold``. Defaults to False.

   .. note::

      ``threshold`` and ``filter_outliers`` are keyword arguments which must be
      explicitly invoked in the function call by writing ``threshold=`` and
      ``filter_outliers=``.

class HaarCascade -- Feature Descriptor
=======================================

The Haar Cascade feature descriptor is used for the ``image.find_features()``
method. It doesn't have any methods itself for you to call.

Constructors
------------

.. class:: image.HaarCascade(path, stages=Auto)

    Loads a Haar Cascade into memory from a Haar Cascade binary file formatted
    for your OpenMV Cam. If you pass "frontalface" instead of a path then this
    constructor will load the built-in frontal face Haar Cascade into memory.
    Additionally, you can also pass "eye" to load a Haar Cascade for eyes into
    memory. Finally, this method returns the loaded Haar Cascade object for use
    with ``image.find_features()``.

    ``stages`` defaults to the number of stages in the Haar Cascade. However,
    you can specify a lower number of stages to speed up processing the feature
    detector at the cost of a higher rate of false positives.

    .. note:: You can make your own Haar Cascades to use with your OpenMV Cam.
              First, Google for "<thing> Haar Cascade" to see if someone
              already made an OpenCV Haar Cascade for an object you want to
              detect. If not... then you'll have to generate your own (which is
              a lot of work). See `here <http://coding-robin.de/2013/07/22/train-your-own-opencv-haar-classifier.html>`_
              for how to make your own Haar Cascade. Then see this `script <https://github.com/openmv/openmv/blob/master/usr/openmv-cascade.py>`_
              for converting OpenCV Haar Cascades into a format your OpenMV Cam
              can read.

    Q: What is a Haar Cascade?

    A: A Haar Cascade is a series of contrast checks that are used to determine
    if an object is present in the image. The contrast checks are split of into
    stages where a stage is only run if previous stages have already passed.
    The contrast checks are simple things like checking if the center vertical
    of the image is lighter than the edges. Large area checks are performed
    first in the earlier stages followed by more numerous and smaller area
    checks in later stages.

    Q: How are Haar Cascades made?

    A: Haar Cascades are made by training the generator algorithm against
    positive and negative labeled images. For example, you'd train the
    generator algorithm against hundreds of pictures with cats in them that
    have been labeled as images with cats and against hundreds of images with
    not cat like things labeled differently. The generator algorithm will then
    produce a Haar Cascade that detects cats.

    .. note::

      ``stages`` is a keyword argument which must be explicitly invoked in the
      function call by writing ``stages=``.

class Histogram -- Histogram Object
===================================

The histogram object is returned by ``image.get_histogram``.

Grayscale histograms have one channel with some number of bins. All bins are
normalized so that all bins sum to 1.

RGB565 histograms have three channels with some number of bins each. All bins
are normalized so that all bins in a channel sum to 1.

Methods
-------

.. method:: histogram.bins()

   Returns a list of floats for the grayscale histogram.

   You may also get this value doing ``[0]`` on the object.

.. method:: histogram.l_bins()

   Returns a list of floats for the RGB565 histogram LAB L channel.

   You may also get this value doing ``[0]`` on the object.

.. method:: histogram.a_bins()

   Returns a list of floats for the RGB565 histogram LAB A channel.

   You may also get this value doing ``[1]`` on the object.

.. method:: histogram.b_bins()

   Returns a list of floats for the RGB565 histogram LAB B channel.

   You may also get this value doing ``[2]`` on the object.

.. method:: histogram.get_percentile(percentile)

   Computes the CDF of the histogram channels and returns a ``percentile``
   object with the values of the histogram at the passed in ``percentile`` (0.0
   - 1.0) (float). So, if you pass in 0.1 this method will tell you (going from
   left-to-right in the histogram) what bin when summed into an accumulator
   caused the accumulator to cross 0.1. This is useful to determine min (with
   0.1) and max (with 0.9) of a color distribution without outlier effects
   ruining your results for adaptive color tracking.

.. method:: histogram.get_statistics()

   Computes the mean, median, mode, standard deviation, min, max, lower
   quartile, and upper quartile of each color channel in the histogram and
   returns a ``statistics`` object.

   You may also use ``histogram.statistics()`` and ``histogram.get_stats()``
   as aliases for this method.

class Percentile -- Percentile Object
=====================================

The percentile object is returned by ``histogram.get_percentile``.

Grayscale percentiles have one channel. Use the non ``l_*``, ``a_*``, and
``b_*`` method.

RGB565 percentiles have three channels. Use the ``l_*``, ``a_*``, and ``b_*``
methods.

Methods
-------

.. method:: percentile.value()

   Return the grayscale percentile value (between 0 and 255).

   You may also get this value doing ``[0]`` on the object.

.. method:: percentile.l_value()

   Return the RGB565 LAB L channel percentile value (between 0 and 100).

   You may also get this value doing ``[0]`` on the object.

.. method:: percentile.a_value()

   Return the RGB565 LAB A channel percentile value (between -128 and 127).

   You may also get this value doing ``[1]`` on the object.

.. method:: percentile.b_value()

   Return the RGB565 LAB B channel percentile value (between -128 and 127).

   You may also get this value doing ``[2]`` on the object.

class Statistics -- Statistics Object
=====================================

The percentile object is returned by ``histogram.get_statistics`` or
``image.get_statistics``.

Grayscale statistics have one channel. Use the non ``l_*``, ``a_*``, and
``b_*`` method.

RGB565 statistics have three channels. Use the ``l_*``, ``a_*``, and ``b_*``
methods.

Methods
-------

.. method:: statistics.mean()

   Returns the grayscale mean (0-255) (int).

   You may also get this value doing ``[0]`` on the object.

.. method:: statistics.median()

   Returns the grayscale median (0-255) (int).

   You may also get this value doing ``[1]`` on the object.

.. method:: statistics.mode()

   Returns the grayscale mode (0-255) (int).

   You may also get this value doing ``[2]`` on the object.

.. method:: statistics.stdev()

   Returns the grayscale standard deviation (0-255) (int).

   You may also get this value doing ``[3]`` on the object.

.. method:: statistics.min()

   Returns the grayscale min (0-255) (int).

   You may also get this value doing ``[4]`` on the object.

.. method:: statistics.max()

   Returns the grayscale max (0-255) (int).

   You may also get this value doing ``[5]`` on the object.

.. method:: statistics.lq()

   Returns the grayscale lower quartile (0-255) (int).

   You may also get this value doing ``[6]`` on the object.

.. method:: statistics.uq()

   Returns the grayscale upper quartile (0-255) (int).

   You may also get this value doing ``[7]`` on the object.

.. method:: statistics.l_mean()

   Returns the RGB565 LAB L mean (0-255) (int).

   You may also get this value doing ``[0]`` on the object.

.. method:: statistics.l_median()

   Returns the RGB565 LAB L median (0-255) (int).

   You may also get this value doing ``[1]`` on the object.

.. method:: statistics.l_mode()

   Returns the RGB565 LAB L mode (0-255) (int).

   You may also get this value doing ``[2]`` on the object.

.. method:: statistics.l_stdev()

   Returns the RGB565 LAB L standard deviation (0-255) (int).

   You may also get this value doing ``[3]`` on the object.

.. method:: statistics.l_min()

   Returns the RGB565 LAB L min (0-255) (int).

   You may also get this value doing ``[4]`` on the object.

.. method:: statistics.l_max()

   Returns the RGB565 LAB L max (0-255) (int).

   You may also get this value doing ``[5]`` on the object.

.. method:: statistics.l_lq()

   Returns the RGB565 LAB L lower quartile (0-255) (int).

   You may also get this value doing ``[6]`` on the object.

.. method:: statistics.l_uq()

   Returns the RGB565 LAB L upper quartile (0-255) (int).

   You may also get this value doing ``[7]`` on the object.

.. method:: statistics.a_mean()

   Returns the RGB565 LAB A mean (0-255) (int).

   You may also get this value doing ``[8]`` on the object.

.. method:: statistics.a_median()

   Returns the RGB565 LAB A median (0-255) (int).

   You may also get this value doing ``[9]`` on the object.

.. method:: statistics.a_mode()

   Returns the RGB565 LAB A mode (0-255) (int).

   You may also get this value doing ``[10]`` on the object.

.. method:: statistics.a_stdev()

   Returns the RGB565 LAB A standard deviation (0-255) (int).

   You may also get this value doing ``[11]`` on the object.

.. method:: statistics.a_min()

   Returns the RGB565 LAB A min (0-255) (int).

   You may also get this value doing ``[12]`` on the object.

.. method:: statistics.a_max()

   Returns the RGB565 LAB A max (0-255) (int).

   You may also get this value doing ``[13]`` on the object.

.. method:: statistics.a_lq()

   Returns the RGB565 LAB A lower quartile (0-255) (int).

   You may also get this value doing ``[14]`` on the object.

.. method:: statistics.a_uq()

   Returns the RGB565 LAB A upper quartile (0-255) (int).

   You may also get this value doing ``[15]`` on the object.

.. method:: statistics.b_mean()

   Returns the RGB565 LAB B mean (0-255) (int).

   You may also get this value doing ``[16]`` on the object.

.. method:: statistics.b_median()

   Returns the RGB565 LAB B median (0-255) (int).

   You may also get this value doing ``[17]`` on the object.

.. method:: statistics.b_mode()

   Returns the RGB565 LAB B mode (0-255) (int).

   You may also get this value doing ``[18]`` on the object.

.. method:: statistics.b_stdev()

   Returns the RGB565 LAB B standard deviation (0-255) (int).

   You may also get this value doing ``[19]`` on the object.

.. method:: statistics.b_min()

   Returns the RGB565 LAB B min (0-255) (int).

   You may also get this value doing ``[20]`` on the object.

.. method:: statistics.b_max()

   Returns the RGB565 LAB B max (0-255) (int).

   You may also get this value doing ``[21]`` on the object.

.. method:: statistics.b_lq()

   Returns the RGB565 LAB B lower quartile (0-255) (int).

   You may also get this value doing ``[22]`` on the object.

.. method:: statistics.b_uq()

   Returns the RGB565 LAB B upper quartile (0-255) (int).

   You may also get this value doing ``[23]`` on the object.

class Blob -- Blob object
=========================

The blob object is returned by ``image.find_blobs``.

Methods
-------

.. method:: blob.rect()

   Returns a rectangle tuple (x, y, w, h) for use with other ``image`` methods
   like ``image.draw_rectangle`` of the blob's bounding box.

.. method:: blob.x()

   Returns the blob's bounding box x coordinate (int).

   You may also get this value doing ``[0]`` on the object.

.. method:: blob.y()

   Returns the blob's bounding box y coordinate (int).

   You may also get this value doing ``[1]`` on the object.

.. method:: blob.w()

   Returns the blob's bounding box w coordinate (int).

   You may also get this value doing ``[2]`` on the object.

.. method:: blob.h()

   Returns the blob's bounding box h coordinate (int).

   You may also get this value doing ``[3]`` on the object.

.. method:: blob.pixels()

   Returns the number of pixels that are part of this blob (int).

   You may also get this value doing ``[4]`` on the object.

.. method:: blob.cx()

   Returns the centroid x position of the blob (int).

   You may also get this value doing ``[5]`` on the object.

.. method:: blob.cy()

   Returns the centroid y position of the blob (int).

   You may also get this value doing ``[6]`` on the object.

.. method:: blob.rotation()

   Returns the rotation of the blob in radians (float). If the blob is like
   a pencil or pen this value will be unique for 0-180 degrees. If the blob
   is round this value is not useful. You'll only be able to get 0-360 degrees
   of rotation from this if the blob has no symmetry at all.

   You may also get this value doing ``[7]`` on the object.

.. method:: blob.code()

   Returns a 16-bit binary number with a bit set in it for each color threshold
   that's part of this blob. For example, if you passed ``image.find_blobs``
   three color thresholds to look for then bits 0/1/2 may be set for this blob.
   Note that only one bit will be set for each blob unless ``image.find_blobs``
   was called with ``merge=True``. Then its possible for multiple blobs with
   different color thresholds to be merged together. You can use this method
   along with multiple thresholds to implement color code tracking.

   You may also get this value doing ``[8]`` on the object.

.. method:: blob.count()

   Returns the number of blobs merged into this blob. THis is 1 unless you
   called ``image.find_blobs`` with ``merge=True``.

   You may also get this value doing ``[9]`` on the object.

.. method:: blob.area()

   Returns the area of the bounding box around the blob. (w * h).

.. method:: blob.density()

   Returns the density ratio of the blob. This is the number of pixels in the
   blob over its bounding box area. A low density ratio means in general that
   the lock on the object isn't very good.

class QRCode -- QRCode object
=============================

The qrcode object is returned by ``image.find_qrcodes``.

.. method:: qrcode.rect()

   Returns a rectangle tuple (x, y, w, h) for use with other ``image`` methods
   like ``image.draw_rectangle`` of the qrcode's bounding box.

.. method:: qrcode.x()

   Returns the qrcode's bounding box x coordinate (int).

   You may also get this value doing ``[0]`` on the object.

.. method:: qrcode.y()

   Returns the qrcode's bounding box y coordinate (int).

   You may also get this value doing ``[1]`` on the object.

.. method:: qrcode.w()

   Returns the qrcode's bounding box w coordinate (int).

   You may also get this value doing ``[2]`` on the object.

.. method:: qrcode.h()

   Returns the qrcode's bounding box h coordinate (int).

   You may also get this value doing ``[3]`` on the object.

.. method:: qrcode.payload()

   Returns the payload string of the qrcode. E.g. the URL.

   You may also get this value doing ``[4]`` on the object.

.. method:: qrcode.version()

   Returns the version number of the qrcode (int).

   You may also get this value doing ``[5]`` on the object.

.. method:: qrcode.ecc_level()

   Returns the ecc_level of the qrcode (int).

   You may also get this value doing ``[6]`` on the object.

.. method:: qrcode.mask()

   Returns the mask of the qrcode (int).

   You may also get this value doing ``[7]`` on the object.

.. method:: qrcode.data_type()

   Returns the data type of the qrcode (int).

   You may also get this value doing ``[8]`` on the object.

.. method:: qrcode.eci()

   Returns the eci of the qrcode (int). The eci stores the encoding of data
   bytes in the QR Code. If you plan to handling QR Codes that contain more
   than just standard ASCII text you will need to look at this value.

   You may also get this value doing ``[9]`` on the object.

.. method:: qrcode.is_numeric()

   Returns True if the data_type of the qrcode is numeric.

.. method:: qrcode.is_alphanumeric()

   Returns True if the data_type of the qrcode is alpha numeric.

.. method:: qrcode.is_binary()

   Returns True if the data_type of the qrcode is binary. If you are serious
   about handling all types of text you need to check the eci if this is True
   to determine the text encoding of the data. Usually, it's just standard
   ASCII, but, it could be UTF8 that has some 2-byte characters in it.

.. method:: qrcode.is_kanji()

   Returns True if the data_type of the qrcode is alpha Kanji. If this is True
   then you'll need to decode the string yourself as Kanji symbols are 10-bits
   per character and MicroPython has no support to parse this kind of text. The
   payload in this case must be treated as just a large byte array.

class AprilTag -- AprilTag object
=================================

The apriltag object is returned by ``image.find_apriltags``.

.. method:: apriltag.rect()

   Returns a rectangle tuple (x, y, w, h) for use with other ``image`` methods
   like ``image.draw_rectangle`` of the apriltag's bounding box.

.. method:: apriltag.x()

   Returns the apriltag's bounding box x coordinate (int).

   You may also get this value doing ``[0]`` on the object.

.. method:: apriltag.y()

   Returns the apriltag's bounding box y coordinate (int).

   You may also get this value doing ``[1]`` on the object.

.. method:: apriltag.w()

   Returns the apriltag's bounding box w coordinate (int).

   You may also get this value doing ``[2]`` on the object.

.. method:: apriltag.h()

   Returns the apriltag's bounding box h coordinate (int).

   You may also get this value doing ``[3]`` on the object.

.. method:: apriltag.id()

   Returns the numeric id of the apriltag.

     * TAG16H5 -> 0 to 29
     * TAG25H7 -> 0 to 241
     * TAG25H9 -> 0 to 34
     * TAG36H10 -> 0 to 2319
     * TAG36H11 -> 0 to 586
     * ARTOOLKIT -> 0 to 511

   You may also get this value doing ``[4]`` on the object.

.. method:: apriltag.family()

   Returns the numeric family of the apriltag.

     * image.TAG16H5
     * image.TAG25H7
     * image.TAG25H9
     * image.TAG36H10
     * image.TAG36H11
     * image.ARTOOLKIT

   You may also get this value doing ``[5]`` on the object.

.. method:: apriltag.cx()

   Returns the centroid x position of the apriltag (int).

   You may also get this value doing ``[6]`` on the object.

.. method:: apriltag.cy()

   Returns the centroid y position of the apriltag (int).

   You may also get this value doing ``[7]`` on the object.

.. method:: apriltag.rotation()

   Returns the rotation of the apriltag in radians (float).

   You may also get this value doing ``[8]`` on the object.

.. method:: apriltag.decision_margin()

   Returns the quality of the apriltag match (0.0 - 1.0) where 1.0 is the best.

   You may also get this value doing ``[9]`` on the object.

.. method:: apriltag.hamming()

   Returns the number of accepted bit errors for this tag.

     * TAG16H5 -> 0 bit errors will be accepted
     * TAG25H7 -> up to 1 bit error may be accepted
     * TAG25H9 -> up to 3 bit errors may be accepted
     * TAG36H10 -> up to 3 bit errors may be accepted
     * TAG36H11 -> up to 4 bit errors may be accepted
     * ARTOOLKIT -> 0 bit errors will be accepted

   You may also get this value doing ``[10]`` on the object.

.. method:: apriltag.goodness()

   Returns the quality of the apriltag image (0.0 - 1.0) where 1.0 is the best.

   .. note::

      This value is always 0.0 for now. We may enable a feature called "tag
      refinement" in the future which will allow detection of small apriltags.
      However, this feature currently drops the frame rate to less than 1 FPS.

   You may also get this value doing ``[11]`` on the object.

.. method:: apriltag.x_translation()

   Returns the translation in unknown units from the camera in the X direction.

   This method is useful for determining the apriltag's location away from the
   camera. However, the size of the apriltag, the lens you are using, etc. all
   come into play as to actually determining what the X units are in. For ease
   of use we recommend you use a lookup table to convert the output of this
   method to something useful for your application.

   Note that this is the left-to-right direction.

   You may also get this value doing ``[12]`` on the object.

.. method:: apriltag.y_translation()

   Returns the translation in unknown units from the camera in the Y direction.

   This method is useful for determining the apriltag's location away from the
   camera. However, the size of the apriltag, the lens you are using, etc. all
   come into play as to actually determining what the Y units are in. For ease
   of use we recommend you use a lookup table to convert the output of this
   method to something useful for your application.

   Note that this is the up-to-down direction.

   You may also get this value doing ``[13]`` on the object.

.. method:: apriltag.z_translation()

   Returns the translation in unknown units from the camera in the Z direction.

   This method is useful for determining the apriltag's location away from the
   camera. However, the size of the apriltag, the lens you are using, etc. all
   come into play as to actually determining what the Z units are in. For ease
   of use we recommend you use a lookup table to convert the output of this
   method to something useful for your application.

   Note that this is the front-to-back direction.

   You may also get this value doing ``[14]`` on the object.

.. method:: apriltag.x_rotation()

   Returns the rotation in radians of the apriltag in the X plane. E.g. moving
   the camera left-to-right while looking at the tag.

   You may also get this value doing ``[15]`` on the object.

.. method:: apriltag.y_rotation()

   Returns the rotation in radians of the apriltag in the Y plane. E.g. moving
   the camera up-to-down while looking at the tag.

   You may also get this value doing ``[16]`` on the object.

.. method:: apriltag.z_rotation()

   Returns the rotation in radians of the apriltag in the Z plane. E.g.
   rotating the camera while looking directly at the tag.

   Note that this is just a renamed version of ``apriltag.rotation()``.

   You may also get this value doing ``[17]`` on the object.

class BarCode -- BarCode object
===============================

The barcode object is returned by ``image.find_barcodes``.

.. method:: barcode.rect()

   Returns a rectangle tuple (x, y, w, h) for use with other ``image`` methods
   like ``image.draw_rectangle`` of the barcode's bounding box.

.. method:: barcode.x()

   Returns the barcode's bounding box x coordinate (int).

   You may also get this value doing ``[0]`` on the object.

.. method:: barcode.y()

   Returns the barcode's bounding box y coordinate (int).

   You may also get this value doing ``[1]`` on the object.

.. method:: barcode.w()

   Returns the barcode's bounding box w coordinate (int).

   You may also get this value doing ``[2]`` on the object.

.. method:: barcode.h()

   Returns the barcode's bounding box h coordinate (int).

   You may also get this value doing ``[3]`` on the object.

.. method:: barcode.payload()

   Returns the payload string of the barcode. E.g. The number.

   You may also get this value doing ``[4]`` on the object.

.. method:: barcode.type()

   Returns the type enumeration of the barcode (int).

   You may also get this value doing ``[5]`` on the object.

     * image.EAN2
     * image.EAN5
     * image.EAN8
     * image.UPCE
     * image.ISBN10
     * image.UPCA
     * image.EAN13
     * image.ISBN13
     * image.I25
     * image.DATABAR
     * image.DATABAR_EXP
     * image.CODABAR
     * image.CODE39
     * image.PDF417 - Future (e.g. doesn't work right now).
     * image.CODE93
     * image.CODE128

.. method:: barcode.rotation()

   Returns the rotation of the barcode in radians (float).

   You may also get this value doing ``[6]`` on the object.

.. method:: barcode.quality()

   Returns the number of times this barcode was detected in the image (int).

   When scanning a barcode each new scanline can decode the same barcode. This
   value increments for a barcode each time that happens...

   You may also get this value doing ``[7]`` on the object.

class Image -- Image object
===========================

The image object is the basic object for machine vision operations.

Constructors
------------

.. class:: image.Image(path, copy_to_fb=False)

   Creates a new image object from a file at ``path``.

   Supports bmp/pgm/ppm/jpg/jpeg image files.

   ``copy_to_fb`` if True the image is loaded directly into the frame buffer
   allowing you to load up large images. If False, the image is loaded into
   MicroPython's heap which is much smaller than the frame buffer.

      *
        On the OpenMV Cam M4 you should try to keep images sizes less than
        8KB in size if ``copy_to_fb`` is False. Otherwise, images can be
        up to 160KB in size.

      *
        On the OpenMV Cam M7 you should try to keep images sizes less than
        16KB in size if ``copy_to_fb`` is False. Otherwise, images can be
        up to 320KB in size.

   Images support "[]" notation. Do ``image[index] = 8/16-bit value`` to assign
   an image pixel or ``image[index]`` to get an image pixel which will be
   either an 8-bit value for grayscale images of a 16-bit RGB565 value for RGB
   images.

   For JPEG images the "[]" allows you to access the compressed JPEG image blob
   as a byte-array. Reading and writing to the data array is opaque however as
   JPEG images are compressed byte streams.

   Images also support read buffer operations. You can pass images to all sorts
   of MicroPython functions like as if the image were a byte-array object. In
   particular, if you'd like to transmit an image you can just pass it to the
   UART/SPI/I2C write functions to be transmitted automatically.

   .. note::

      ``copy_to_fb`` is a keyword argument which must be explicitly invoked in
      the function call by writing ``copy_to_fb=``.

Methods
-------

.. method:: image.copy(roi=Auto)

   Creates a copy of the image object.

   ``roi`` is the region-of-interest rectangle (x, y, w, h) to copy from.
   If not specified, it is equal to the image rectangle which copies the entire
   image. This argument is not applicable for JPEG images.

   Keep in mind that image copies are stored in the MicroPython heap and not
   the frame buffer. As such, you need to keep image copies under 8KB for the
   OpenMV Cam M4 and 16KB for the OpenMV Cam M7. If you attempt a copy
   operation that uses up all the heap space this function will throw an
   exception. Since images are large this is rather easy to trigger.

   .. note::

      ``roi`` is a keyword argument which must be explicitly invoked in
      the function call by writing ``roi=``.

.. method:: image.save(path, roi=Auto, quality=50)

   Saves a copy of the image to the filesystem at ``path``.

   Supports bmp/pgm/ppm/jpg/jpeg image files. Note that you cannot save jpeg
   compressed images to an uncompressed format.

   ``roi`` is the region-of-interest rectangle (x, y, w, h) to copy from.
   If not specified, it is equal to the image rectangle which copies the entire
   image. This argument is not applicable for JPEG images.

   ``quality`` is the jpeg compression quality to use to save the image to jpeg
   format if the image is not already compressed.

   .. note::

      ``roi`` and ``quality`` are keyword arguments which must be explicitly
      invoked in the function call by writing ``roi=`` or ``quality=``.

.. method:: image.compress(quality=50)

   JPEG compresses the image in place. Use this method versus ``compressed``
   to save heap space and to use a higher ``quality`` for compression at the
   cost of destroying the original image.

   ``quality`` is the compression quality (0-100) (int).

   .. note::

      ``quality`` is a keyword argument which must be explicitly
      invoked in the function call by writing ``quality=``.

   Only call this on Grayscale and RGB565 images.

.. method:: image.compress_for_ide(quality=50)

   JPEG compresses the image in place. Use this method versus ``compressed``
   to save heap space and to use a higher ``quality`` for compression at the
   cost of destroying the original image.

   This method JPEG compresses the image and then formats the JPEG data for
   transmission to OpenMV IDE to display by encoding every 6-bits as a byte
   valued between 128-191. This is done to prevent JPEG data from being
   misinterpreted as other text data in the byte stream.

   You need to use this method to format image data for display to terminal
   windows created via "Open Terminal" in OpenMV IDE.

   ``quality`` is the compression quality (0-100) (int).

   .. note::

      ``quality`` is a keyword argument which must be explicitly
      invoked in the function call by writing ``quality=``.

   Only call this on Grayscale and RGB565 images.

.. method:: image.compressed(quality=50)

   Returns a JPEG compressed image - the original image is untouched. However,
   this method requires a somewhat large allocation of heap space so the image
   compression quality must be low and the image resolution must be low.

   ``quality`` is the compression quality (0-100) (int).

   .. note::

      ``quality`` is a keyword argument which must be explicitly
      invoked in the function call by writing ``quality=``.

   Only call this on Grayscale and RGB565 images.

.. method:: image.compressed_for_ide(quality=50)

   Returns a JPEG compressed image - the original image is untouched. However,
   this method requires a somewhat large allocation of heap space so the image
   compression quality must be low and the image resolution must be low.

   This method JPEG compresses the image and then formats the JPEG data for
   transmission to OpenMV IDE to display by encoding every 6-bits as a byte
   valued between 128-191. This is done to prevent JPEG data from being
   misinterpreted as other text data in the byte stream.

   You need to use this method to format image data for display to terminal
   windows created via "Open Terminal" in OpenMV IDE.

   ``quality`` is the compression quality (0-100) (int).

   .. note::

      ``quality`` is a keyword argument which must be explicitly
      invoked in the function call by writing ``quality=``.

   Only call this on Grayscale and RGB565 images.

.. method:: image.width()

   Returns the image width in pixels.

.. method:: image.height()

   Returns the image height in pixels.

.. method:: image.format()

   Returns ``sensor.GRAYSCALE`` for grayscale images, ``sensor.RGB565`` for RGB
   images and ``sensor.JPEG`` for JPEG images.

.. method:: image.size()

   Returns the image size in bytes.

.. method:: image.clear()

   Zeros all bytes in GRAYSCALE or RGB565 images. Do not call this method on
   JPEG images.

.. method:: image.get_pixel(x, y)

   For grayscale images: Returns the grayscale pixel value at location (x, y).
   For RGB images: Returns the rgb888 pixel tuple (r, g, b) at location (x, y).

   Not supported on compressed images.

.. method:: image.set_pixel(x, y, pixel)

   For grayscale images: Sets the pixel at location (x, y) to the grayscale
   value ``pixel``.
   For RGB images: Sets the pixel at location (x, y) to the rgb888 tuple
   (r, g, b) ``pixel``.

   Not supported on compressed images.

.. method:: image.draw_line(line_tuple, color=White)

   Draws a line using the ``line_tuple`` (x0, y0, x1, y1) from (x0, y0) to
   (x1, y1) on the image.

   ``color`` is an int value (0-255) for grayscale images and a RGB888 tuple
   (r, g, b) for RGB images. Defaults to white.

   Not supported on compressed images.

   .. note::

      ``color`` is a keyword argument which must be explicitly
      invoked in the function call by writing ``color=``.

.. method:: image.draw_rectangle(rect_tuple, color=White)

   Draws an unfilled rectangle using the ``rect_tuple`` (x, y, w, h) on the
   image.

   ``color`` is an int value (0-255) for grayscale images and a RGB888 tuple
   (r, g, b) for RGB images. Defaults to white.

   Not supported on compressed images.

   .. note::

      ``color`` is a keyword argument which must be explicitly
      invoked in the function call by writing ``color=``.

.. method:: image.draw_circle(x, y, radius, color=White)

   Draws an unfilled circle at (``x``, ``y``) with integer ``radius`` on the
   image.

   ``color`` is an int value (0-255) for grayscale images and a RGB888 tuple
   (r, g, b) for RGB images. Defaults to white.

   Not supported on compressed images.

   .. note::

      ``color`` is a keyword argument which must be explicitly
      invoked in the function call by writing ``color=``.

.. method:: image.draw_string(x, y, text, color=White)

   Draws 8x10 text starting at (``x``, ``y``) using ``text`` on the image.
   ``\n``, ``\r``, and ``\r\n`` line endings move the cursor to the next line.

   ``color`` is an int value (0-255) for grayscale images and a RGB888 tuple
   (r, g, b) for RGB images. Defaults to white.

   Not supported on compressed images.

   .. note::

      ``color`` is a keyword argument which must be explicitly
      invoked in the function call by writing ``color=``.

.. method:: image.draw_cross(x, y, size=5, color=White)

   Draws a cross at (``x``, ``y``) whose sides are ``size`` (int) long on the
   image.

   ``color`` is an int value (0-255) for grayscale images and a RGB888 tuple
   (r, g, b) for RGB images. Defaults to white.

   Not supported on compressed images.

   .. note::

      ``size`` and ``color`` are keyword arguments which must be explicitly
      invoked in the function call by writing ``size=`` or ``color=``.

.. method:: image.draw_keypoints(keypoints, size=Auto, color=White)

   Draws the keypoints of a keypoints object on the image. ``size`` controls
   the size of the keypoints and is scaled to look good on the image unless
   overridden.

   ``color`` is an int value (0-255) for grayscale images and a RGB888 tuple
   (r, g, b) for RGB images. Defaults to white.

   Not supported on compressed images.

   .. note::

      ``size`` and ``color`` are keyword arguments which must be explicitly
      invoked in the function call by writing ``size=`` or ``color=``.

.. method:: image.binary(thresholds, invert=False)

   For grayscale images ``thresholds`` is a list of (lower, upper) grayscale
   pixel thresholds to segment the image by. Segmentation converts all pixels
   within the thresholds to 1 (white) and all pixels outside to 0 (black).

   For RGB images ``thresholds`` is a list of (l_lo, l_hi, a_lo, a_hi, b_lo,
   b_hi) LAB pixel thresholds to segment the image by. Segmentation converts
   all pixels within the thresholds to 1 (white) and all pixels outside to 0
   (black).

   Lo/Hi thresholds being swapped is automatically handled.

   ``invert`` inverts the outcome of the segmentation operation.

   Not supported on compressed images.

   .. note::

      ``invert`` is a keyword argument which must be explicitly
      invoked in the function call by writing ``invert=``.

.. method:: image.invert()

   Inverts the binary image 0 (black) pixels go to 1 (white) and 1 (white)
   pixels go to 0 (black).

   Not supported on compressed images.

.. method:: image.nand(image)

   Logically NANDs this image with another image.

   ``image`` can either be an image object or a path to an uncompressed image
   file (bmp/pgm/ppm).

   Both images must be the same size and the same type (grayscale/rgb).

   Not supported on compressed images.

.. method:: image.nor(image)

   Logically NORs this image with another image.

   ``image`` can either be an image object or a path to an uncompressed image
   file (bmp/pgm/ppm).

   Both images must be the same size and the same type (grayscale/rgb).

   Not supported on compressed images.

.. method:: image.xor(image)

   Logically XORs this image with another image.

   ``image`` can either be an image object or a path to an uncompressed image
   file (bmp/pgm/ppm).

   Both images must be the same size and the same type (grayscale/rgb).

   Not supported on compressed images.

.. method:: image.xnor(image)

   Logically XNORs this image with another image.

   ``image`` can either be an image object or a path to an uncompressed image
   file (bmp/pgm/ppm).

   Both images must be the same size and the same type (grayscale/rgb).

   Not supported on compressed images.

.. method:: image.erode(size, threshold=Auto)

   Removes pixels from the edges of segmented areas.

   This method works by convolving a kernel of ((size*2)+1)x((size*2)+1) pixels
   across the image and zeroing the center pixel of the kernel if the sum of
   the neighbour pixels set is not greater than ``threshold``.

   This method works like the standard erode method if threshold is not set. If
   ``threshold`` is set then you can specify erode to only erode pixels that
   have, for example, less than 2 pixels set around them with a threshold of 2.

   Not supported on compressed images. This method is designed to work on
   binary images.

   .. note::

      ``threshold`` is a keyword argument which must be explicitly
      invoked in the function call by writing ``threshold=``.

.. method:: image.dilate(size, threshold=Auto)

   Adds pixels to the edges of segmented areas.

   This method works by convolving a kernel of ((size*2)+1)x((size*2)+1) pixels
   across the image and setting the center pixel of the kernel if the sum of
   the neighbour pixels set is greater than ``threshold``.

   This method works like the standard dilate method if threshold is not set.
   If ``threshold`` is set then you can specify dilate to only dilate pixels
   that have, for example, more than 2 pixels set around them with a threshold
   of 2.

   Not supported on compressed images. This method is designed to work on
   binary images.

   .. note::

      ``threshold`` is a keyword argument which must be explicitly
      invoked in the function call by writing ``threshold=``.

.. method:: image.negate()

   Numerically inverts pixel values for each color channel. E.g. (255-pixel).

   Not supported on compressed images.

.. method:: image.difference(image)

   Subtracts another image from this image. E.g. for each color channel each
   pixel is replaced with ABS(this.pixel-image.pixel).

   ``image`` can either be an image object or a path to an uncompressed image
   file (bmp/pgm/ppm).

   Both images must be the same size and the same type (grayscale/rgb).

   Not supported on compressed images.

   .. note:: This function is used for frame differencing which you can then
             use to do motion detection. You can then mask the resulting image
             using NAND/NOR before running statistics functions on the image.

.. method:: image.replace(image)

   Replace this image with ``image`` (this is much faster than blend for this).

   ``image`` can either be an image object or a path to an uncompressed image
   file (bmp/pgm/ppm).

   Both images must be the same size and the same type (grayscale/rgb).

   Not supported on compressed images.

.. method:: image.blend(image, alpha=128)

   Blends another image ``image`` into this image.

   ``image`` can either be an image object or a path to an uncompressed image
   file (bmp/pgm/ppm).

   ``alpha`` controls the transparency. 256 for an opaque overlay. 0 for none.

   Both images must be the same size and the same type (grayscale/rgb).

   Not supported on compressed images.

   .. note::

      ``alpha`` is a keyword argument which must be explicitly
      invoked in the function call by writing ``alpha=``.

.. method:: image.morph(size, kernel, mul=Auto, add=0)

   Convolves the image by a filter kernel.

   ``size`` controls the size of the kernel which must be
   ((size*2)+1)x((size*2)+1) pixels big.

   ``kernel`` is the kernel to convolve the image by. It can either be a tuple
   or a list of [-128:127] values.

   ``mul`` is number to multiply the convolution pixel result by. When not set
   it defaults to a value that will prevent scaling in the convolution output.

   ``add`` is a value to add to each convolution pixel result.

   ``mul`` basically allows you to do a global contrast adjustment and ``add``
   allows you to do a global brightness adjustment.

   .. note::

      ``mul`` and ``add`` are keyword arguments which must be explicitly
      invoked in the function call by writing ``mul=`` or ``add=``.

.. method:: image.midpoint(size, bias=0.5)

   Runs the midpoint filter on the image.

   ``size`` is the kernel size. Use 1 (3x3 kernel), 2 (5x5 kernel), or higher.

   ``bias`` controls the min/max mixing. 0 for min filtering only, 1.0 for max
   filtering only. By using the ``bias`` you can min/max filter the image.

   Not supported on compressed images.

   .. note::

      ``bias`` is a keyword argument which must be explicitly
      invoked in the function call by writing ``bias=``.

.. method:: image.mean(size)

   Standard mean blurring filter (faster than using morph for this).

   ``size`` is the kernel size. Use 1 (3x3 kernel), 2 (5x5 kernel), or higher.

   Not supported on compressed images.

.. method:: median(size, percentile=0.5)

   Runs the median filter on the image. The median filter is the best filter
   for smoothing surfaces while preserving edges but it is very slow.

   ``size`` is the kernel size. Use 1 (3x3 kernel) or 2 (5x5 kernel).

   ``percentile`` controls the percentile of the value used in the kernel. By
   default each pixel is replace with the 50th percentile (center) of it's
   neighbours. You can set this to 0 for a min filter, 0.25 for a lower quartile
   filter, 0.75 for an upper quartile filter, and 1.0 for a max filter.

   Not supported on compressed images.

   .. note::

      ``percentile`` is a keyword argument which must be explicitly
      invoked in the function call by writing ``percentile=``.

.. method:: image.mode(size)

   Runs the mode filter on the image by replacing each pixel with the mode of
   their neighbours. This method works great on grayscale images. However, on
   RGB images it creates a lot of artifacts on edges because of the non-linear
   nature of the operation.

   ``size`` is the kernel size. Use 1 (3x3 kernel) or 2 (5x5 kernel).

   Not supported on compressed images.

.. method:: image.gaussian(size)

   Smooths the image with the gaussian kernel. ``size`` may be either 3 or 5
   for a 3x3 or 5x5 kernel.

   Not supported on compressed images.

.. method:: image.histeq()

   Runs the histogram equalization algorithm on the image. Histogram
   equalization normalizes the contrast and brightness in the image.

   Not supported on compressed images.

.. method:: image.lens_corr(strength=1.8, zoom=1.0)

   Performs lens correction to un-fisheye the image due to the lens.

   ``strength`` is a float defining how much to un-fisheye the image. Try 1.8
   out by default and then increase or decrease from there until the image
   looks good.

   ``zoom`` is the amount to zoom in on the image by. 1.0 by default.

.. method:: image.get_histogram(roi=Auto, bins=Auto, l_bins=Auto, a_bins=Auto, b_bins=Auto)

   Computes the normalized histogram on all color channels for an ``roi`` and
   returns a ``histogram`` object. Please see the ``histogram`` object for more
   information. You can also invoke this method by using ``image.get_hist`` or
   ``image.histogram``.

   Unless you need to do something advanced with color statistics just use the
   ``image.get_statistics`` method instead of this method for looking at pixel
   areas in an image.

   ``roi`` is the region-of-interest rectangle tuple (x, y, w, h). If not
   specified, it is equal to the image rectangle. Only pixels within the
   ``roi`` are operated on.

   ``bin_count`` and others are the number of bins to use for the histogram
   channels. For grayscale images use ``bin_count`` and for RGB565 images use
   the others for each channel. The bin counts must be greater than 2 for each
   channel. Additionally, it makes no sense to set the bin count larger than
   the number of unique pixel values for each channel.

   Not supported on compressed images.

   .. note::

      ``roi``, ``bin_count``, and etc. are keyword arguments which must be
      explicitly invoked in the function call by writing ``roi=``, etc.

.. method:: image.get_statistics(roi=Auto, bins=Auto, l_bins=Auto, a_bins=Auto, b_bins=Auto)

   Computes the mean, median, mode, standard deviation, min, max, lower
   quartile, and upper quartile for all color channels for an ``roi`` and
   returns a ``statistics`` object. Please see the ``statistics`` object for
   more information. You can also invoke this method by using
   ``image.get_stats`` or ``image.statistics``.

   You'll want to use this method any time you need to get information about
   the values of an area of pixels in an image. For example, after if you're
   trying to detect motion using frame differencing you'll want to use this
   method to determine a change in the color channels of the image to trigger
   your motion detection threshold.

   ``roi`` is the region-of-interest rectangle tuple (x, y, w, h). If not
   specified, it is equal to the image rectangle. Only pixels within the
   ``roi`` are operated on.

   ``bin_count`` and others are the number of bins to use for the histogram
   channels. For grayscale images use ``bin_count`` and for RGB565 images use
   the others for each channel. The bin counts must be greater than 2 for each
   channel. Additionally, it makes no sense to set the bin count larger than
   the number of unique pixel values for each channel.

   Not supported on compressed images.

   .. note::

      ``roi``, ``bin_count``, and etc. are keyword arguments which must be
      explicitly invoked in the function call by writing ``roi=``, etc.

.. method:: image.find_blobs(thresholds, roi=Auto, x_stride=2, y_stride=1, invert=False, area_threshold=10, pixels_threshold=10, merge=False, margin=0, threshold_cb=None, merge_cb=None)

   Finds all blobs (connected pixel regions that pass a threshold test) in the
   image and returns a list of ``blob`` objects which describe each blob.
   Please see the ``blob`` object more more information.

   ``thresholds`` must be a list of tuples
   ``[(lo, hi), (lo, hi), ..., (lo, hi)]`` defining the ranges of color you
   want to track. You may pass up to 16 threshold tuples in one
   ``image.find_blobs`` call. For grayscale images each tuple needs to contain
   two values - a min grayscale value and a max grayscale value. Only pixel
   regions that fall between these thresholds will be considered. For RGB565
   images each tuple needs to have six values (l_lo, l_hi, a_lo, a_hi, b_lo,
   b_hi) - which are minimums and maximums for the LAB L, A, and B channels
   respectively. To easy usage this function will automatically fix swapped
   min and max values. Additionally, a tuple is larger than six values the
   rest are ignored. Conversely, if the tuple is too short the rest of the
   thresholds are assumed to be zero.

   .. note::

      To get the thresholds for the object you want to track just select (click
      and drag) on the object you want to track. The histogram will then update
      to just be in that area. Then just write down where the color
      distribution starts and falls off in each histogram channel. These will
      be your low and high values for ``thresholds``. It's best to manually
      determine the thresholds versus using the upper and lower quartile
      statistics because they are too tight.

   ``roi`` is the region-of-interest rectangle tuple (x, y, w, h). If not
   specified, it is equal to the image rectangle. Only pixels within the
   ``roi`` are operated on.

   ``x_stride`` is the number of pixels to skip when searching for

   ``invert`` inverts the thresholding operation such that instead of matching
   pixels inside of some known color bounds pixels are matched that are outside
   of the known color bounds.

   If a blob's bounding box area is less than ``area_threshold`` it is filtered
   out.

   If a blob's pixel count is less than ``pixel_threshold`` it is filtered out.

   ``merge`` if True merges all not filtered out blobs who's bounding
   rectangles intersect each other. ``margin`` can be used to increase or
   decrease the size of the bounding rectangles for blobs during the
   intersection test. For example, with a margin of 1 blobs who's bounding
   rectangles are 1 pixel away from each other will be merged.

   Merging blobs allows you to implement color code tracking. Each blob object
   has a ``code`` value which is a bit vector made up of 1s for each color
   threshold. For example, if you pass ``image.find_blobs`` two color
   thresholds then the first threshold has a code of 1 and the second 2 (a
   third threshold would be 4 and a fourth would be 8 and so on). Merged blobs
   logically OR all their codes together so that you know what colors produced
   them. This allows you to then track two colors if you get a blob object
   back with two colors then you know it might be a color code.

   You might also want to merge blobs if you are using tight color bounds which
   do not fully track all the pixels of an object you are trying to follow.

   Finally, if you want to merge blobs, but, don't want two color thresholds to
   be merged then just call ``image.find_blobs`` twice with separate thresholds
   so that blobs aren't merged.

   ``threshold_cb`` may be set to the function to call on every blob after its
   been thresholded to filter it from the list of blobs to be merged. The call
   back function will receive one argument - the blob object to be filtered.
   The call back then must return True to keep the blob and False to filter it.

   ``merge_cb`` may be set to the function to call on every two blobs about to
   be merged to prevent or allow the merge. The call back function will receive
   two arguments - the two blob objects to be merged. The call back then must
   return True to merge the blobs or False to prevent merging the blobs.

   Not supported on compressed images.

   .. note::

      All the arguments except ``thresholds`` are keyword arguments and must
      be explicitly invoked with their name and an equal sign.

.. method:: image.find_qrcodes(roi=Auto)

   Finds all qrcodes within the ``roi`` and returns a list of ``qrcode``
   objects. Please see the ``qrcode`` object for more information.

   QR Codes need to be relatively flat in the image for this method to work.
   You can achieve a flatter image that is not effected by lens distortion by
   either using the ``sensor.set_windowing`` function to zoom in the on the
   center of the lens, ``image.lens_corr`` to undo lens barrel distortion, or
   by just changing out the lens for something with a narrower fields of view.
   There are machine vision lenses available which do not cause barrel
   distortion but they are much more expensive to than the standard lenses
   supplied by OpenMV so we don't stock them (since they wouldn't sell).

   ``roi`` is the region-of-interest rectangle tuple (x, y, w, h). If not
   specified, it is equal to the image rectangle. Only pixels within the
   ``roi`` are operated on.

   Not supported on compressed images.

   .. note::

      ``roi`` is a keyword argument which must be explicitly invoked in the
      function call by writing ``roi=``.

.. method:: image.find_apriltags(roi=Auto, families=image.TAG36H11, fx=Auto, fy=Auto, cx=Auto, cy=Auto)

   Finds all apriltags within the ``roi`` and returns a list of ``apriltag``
   objects. Please see the ``apriltag`` object for more information.

   Unlike QR Codes, AprilTags can be detected at much farther distances, worse
   lighting, in warped images, etc. AprilTags are robust too all kinds of
   image distortion issues that QR Codes are not to. That said, AprilTags
   can only encode a numeric ID as their payload.

   AprilTags can also be used for localization purposes. Each ``apriltag``
   object returns its translation and rotation from the camera. The units
   of the translation are determined by ``fx``, ``fy``, ``cx``, and ``cy``
   which are the focal lengths and center points of the image in the X and
   Y directions respectively.

   .. note::

      To create AprilTags use the tag generator tool built-in to OpenMV IDE.
      The tag generator can create printable 8.5"x11" AprilTags.

   ``roi`` is the region-of-interest rectangle tuple (x, y, w, h). If not
   specified, it is equal to the image rectangle. Only pixels within the
   ``roi`` are operated on.

   ``families`` is bitmask of tag families to decode. It is the logical OR of:

     * image.TAG16H5
     * image.TAG25H7
     * image.TAG25H9
     * image.TAG36H10
     * image.TAG36H11
     * image.ARTOOLKIT

   By default it is just ``image.TAG36H11`` which is the best tag family to
   use. Note that ``find_apriltags`` slows down a bit per enabled tag family.

   ``fx`` is the camera X focal length in pixels. For the standard OpenMV Cam
   this is (2.8 / 3.984) * 656. Which is the lens focal length in mm, divided
   by the camera sensor length in the X direction multiplied by the number of
   camera sensor pixels in the X direction (for the OV7725 camera).

   ``fx`` is the camera Y focal length in pixels. For the standard OpenMV Cam
   this is (2.8 / 2.952) * 488. Which is the lens focal length in mm, divided
   by the camera sensor length in the Y direction multiplied by the number of
   camera sensor pixels in the Y direction (for the OV7725 camera).

   ``cx`` is the image center which is just ``image.width()/2``. This is not
   ``roi.w()/2``.

   ``cy`` is the image center which is just ``image.height()/2``. This is not
   ``roi.h()/2``.

   Not supported on compressed images.

   Only supported on the OpenMV Cam M7 or better (not enough RAM on the M4).

   .. note::

      ``roi``, ``families``, ``fx``, ``fy``, ``cx``, and ``cy`` are keyword
      arguments which must be explicitly invoked in the function call by
      writing ``roi=``, ``families=``, ``fx=``, ``fy=``, ``cx=``, and ``cy=``.

.. method:: image.find_barcodes(roi=Auto)

   Finds all 1D barcodes within the ``roi`` and returns a list of ``barcode``
   objects. Please see the ``barcode`` object for more information.

   For best results use a 640 by 40/80/160 window. The lower the vertical res
   the faster everything will run. Since bar codes are linear 1D images you
   just need a lot of resolution in one direction and just a little resolution
   in the other direction. Note that this function scans both horizontally and
   vertically so use can use a 40/80/160 by 480 window if you want. Finally,
   make sure to adjust your lens so that the bar code is positioned where the
   focal length produces the sharpest image. Blurry bar codes can't be decoded.

   This function supports all these 1D barcodes (basically all barcodes):

     * image.EAN2
     * image.EAN5
     * image.EAN8
     * image.UPCE
     * image.ISBN10
     * image.UPCA
     * image.EAN13
     * image.ISBN13
     * image.I25
     * image.DATABAR (RSS-14)
     * image.DATABAR_EXP (RSS-Expanded)
     * image.CODABAR
     * image.CODE39
     * image.PDF417
     * image.CODE93
     * image.CODE128

   ``roi`` is the region-of-interest rectangle tuple (x, y, w, h). If not
   specified, it is equal to the image rectangle. Only pixels within the
   ``roi`` are operated on.

   Not supported on compressed images.

   .. note::

      ``roi`` is a keyword argument which must be explicitly invoked in the
      function call by writing ``roi=``.

.. method:: image.midpoint_pooled(x_div, y_div, bias=0.5)

   Finds the midpoint of ``x_div`` * ``y_div`` squares in the image and returns
   a new image composed of the midpoint of each square.

   A ``bias`` of 0 returns the min of each area while a ``bias`` of 1.0 returns
   the max of each area.

   This methods is useful for preparing images for phase_correlation.

   Not supported on compressed images.

   .. note::

      ``bias`` is a keyword argument which must be explicitly
      invoked in the function call by writing ``bias=``.

.. method:: image.mean_pooled(x_div, y_div, bias=0.5)

   Finds the mean of ``x_div`` * ``y_div`` squares in the image and returns
   a new image composed of the mean of each square.

   This methods is useful for preparing images for phase_correlation.

   Not supported on compressed images.

.. method:: image.find_template(template, threshold, roi=Auto, step=2, search=image.SEARCH_EX)

   Tries to find the first location in the image where template matches using
   Normalized Cross Correlation. Returns a bounding box tuple (x, y, w, h) for
   the matching location otherwise None.

   ``template`` is a small image object that is matched against this image
   object. Note that both images must be grayscale.

   ``threshold`` is floating point number (0.0-1.0) where a higher threshold
   prevents false positives while lowering the detection rate while a lower
   threshold does the opposite.

   ``roi`` is the region-of-interest rectangle (x, y, w, h) to search in.

   ``step`` is the number of pixels to skip past while looking for the
   template. Skipping pixels considerably speeds the algorithm up. This only
   affects the algorithm in SERACH_EX mode.

   ``search`` can be either ``image.SEARCH_DS`` or ``image.SEARCH_EX``.
   ``image.SEARCH_DS`` searches for the template using as faster algorithm
   than ``image.SEARCH_EX`` but may not find the template if it's near the
   edges of the image. ``image.SEARCH_EX`` does an exhaustive search for the
   image but can be much slower than ``image.SEARCH_DS``.

   .. note::

      ``roi``, ``step``, and ``search`` are keyword arguments which must be
      explicitly invoked in the function call by writing ``roi=``, ``step=``,
      or ``search=``.

.. method:: image.find_displacement(template)

   Find the translation offset of the this image from the template. This
   method can be used to do optical flow. This method returns a tuple with
   three values (x_offset, y_offset, response). Where ``x_offset`` is a
   floating point value of the translation in x pixels between each image.
   ``y_offset`` is thus the floating point y translation between images.
   ``response`` is a floating point confidence value that ranges between
   0.0 and 1.0. As the confidence value falls you should trust ``x_offset``
   and ``y_offset`` less. In general, as long as the ``response`` is above
   0.2 or so it's okay. When the ``response`` starts to fall it falls rapidly.

   Note that this algorithm requires a large amount of temporary scratch memory
   to turn and thus won't work on images larger than 32x32 pixels or so on the
   OpenMV Cam M4. You can work on larger images with the OpenMV Cam M7. You
   should use the pooling functions to shrink both images before calling this
   method to get the displacement.

.. method:: image.find_features(cascade, roi=Auto, threshold=0.5, scale=1.5)

   This method searches the image for all areas that match the passed in Haar
   Cascade and returns a list of bounding box rectangles tuples (x, y, w, h)
   around those features. Returns an empty list if no features are found.

   ``cascade`` is a Haar Cascade object. See ``image.HaarCascade()`` for more
   details.

   ``roi`` is the region-of-interest rectangle (x, y, w, h) to work in.
   If not specified, it is equal to the image rectangle.

   ``threshold`` is a threshold (0.0-1.0) where a smaller value increase the
   detection rate while raising the false positive rate. Conversely, a higher
   value decreases the detection rate while lowering the false positive rate.

   ``scale`` is a float that must be greater than 1.0. A higher scale
   factor will run faster but will have much poorer image matches. A good
   value is between 1.35 and 1.5.

   Not supported on compressed images.

   .. note::

      ``roi``, ``threshold`` and ``scale`` are keyword arguments which must be
      explicitly invoked in the function call by writing ``roi``,
      ``threshold=`` or ``scale=``.

.. method:: image.find_eye(roi)

   Searches for the pupil in a region-of-interest (x, y, w, h) tuple around an
   eye. Returns a tuple with the (x, y) location of the pupil in the image.
   Returns (0,0) if no pupils are found.

   To use this function first use ``image.find_features`` with the
   ``frontalface`` HaarCascade to find someone's face. Then use
   ``image.find_features`` with the ``eye`` HaarCascade to find the eyes on the
   face. Finally, call this method on each eye roi returned by
   ``image.find_features`` to get the pupil coordinates.

   Only for grayscale images.

.. method:: image.find_lbp(roi)

   Extracts LBP (local-binary-patterns) keypoints from the region-of-interest
   (x, y, w, h) tuple. You can then use then use the ``image.match_descriptor``
   function to compare two sets of keypoints to get the matching distance.

   Only for grayscale images.

.. method:: image.find_keypoints(roi=Auto, threshold=20, normalized=False, scale_factor=1.5, max_keypoints=100, corner_detector=CORNER_AGAST)

   Extracts ORB keypoints from the region-of-interest (x, y, w, h) tuple. You
   can then use then use the ``image.match_descriptor`` function to compare
   two sets of keypoints to get the matching areas. Returns None if no
   keypoints were found.

   ``threshold`` is a number (between 0 - 255) which controls the number of
   extracted corners. For the default AGAST corner detector this should be
   around 20. FOr the FAST corner detector this should be around 60-80. The
   lower the threshold the more extracted corners you get.

   ``normalized`` is a boolean value that if True turns off extracting
   keypoints at multiple resolutions. Set this to true if you don't care
   about dealing with scaling issues and want the algorithm to run faster.

   ``scale_factor`` is a float that must be greater than 1.0. A higher scale
   factor will run faster but will have much poorer image matches. A good
   value is between 1.35 and 1.5.

   ``max_keypoints`` is the maximum number of keypoints a keypoint object may
   hold. If keypoint objects are too big and causing out of RAM issues then
   decrease this value.

   ``corner_detector`` is the corner detector algorithm to use which extracts
   keypoints from the image. It can be either ``image.FAST`` or
   ``image.AGAST``. The FAST corner detector is faster but much less accurate.

   Only for grayscale images.

   .. note::

      ``roi``, ``threshold``, ``normalized``m ``scale_factor``, ``max_keypoints``,
      and ``corner_detector`` are keyword argument which must be explicitly
      invoked in the function call by writing ``roi=``, ``threshold=``, etc.

.. method:: image.find_lines(roi=Auto, threshold=50)

   For grayscale images only. Finds the lines in a edge detected image using
   the Hough Transform. Returns a list of line tuples (x0, y0, x1, y1).

   ``roi`` is the region-of-interest rectangle (x, y, w, h) to work in.
   If not specified, it is equal to the image rectangle.

   ``threshold`` may be between 0-255. The lower the threshold the more lines
   are pulled out of the image.

   Only for grayscale images.

   .. note::

      ``roi`` and ``threshold`` are keyword argument which must be explicitly
      invoked in the function call by writing ``roi=`` and ``threshold=``.

.. method:: image.find_edges(edge_type, threshold=[100,200])

   For grayscale images only. Does edge detection on the image and replaces the
   image with an image that only has edges. ``edge_type`` can either be:

      * image.EDGE_SIMPLE - Simple thresholded high pass filter algorithm.
      * image.EDGE_CANNY - Canny edge detection algorithm.

   ``threshold`` is a two valued tuple containing a low threshold and high
   threshold. You can control the quality of edges by adjusting these values.

   Only for grayscale images.

   .. note::

      ``threshold`` is keyword argument which must be explicitly invoked in the
      function call by writing ``threshold=``.

Constants
---------

.. data:: image.LBP

   Switch for descriptor functions for LBP.

.. data:: image.ORB

   Switch for descriptor functions for ORB.

.. data:: image.SEARCH_EX

   Exhaustive template matching search.

.. data:: image.SEARCH_DS

   Faster template matching search.

.. data:: image.EDGE_CANNY

   Use the canny edge detection algorithm for doing edge detection on an image.

.. data:: image.EDGE_SIMPLE

   Use a simple thresholded high pass filter algorithm for doing edge detection
   on an image.

.. data:: image.CORNER_FAST

   Faster and less accurate corner detection algorithm for ORB keypoints.

.. data:: image.CORNER_AGAST

   Slower and more accurate corner detection algorithm for ORB keypoints.

.. data:: image.TAG16H5

   TAG1H5 tag family bit mask enum. Used for AprilTags.

.. data:: image.TAG25H7

   TAG25H7 tag family bit mask enum. Used for AprilTags.

.. data:: image.TAG25H9

   TAG25H9 tag family bit mask enum. Used for AprilTags.

.. data:: image.TAG36H10

   TAG36H10 tag family bit mask enum. Used for AprilTags.

.. data:: image.TAG36H11

   TAG36H11 tag family bit mask enum. Used for AprilTags.

.. data:: image.ARTOOLKIT

   ARTOOLKIT tag family bit mask enum. Used for AprilTags.

.. data:: image.EAN2

   EAN2 barcode type enum.

.. data:: image.EAN5

   EAN5 barcode type enum.

.. data:: image.EAN8

   EAN8 barcode type enum.

.. data:: image.UPCE

   UPCE barcode type enum.

.. data:: image.ISBN10

   ISBN10 barcode type enum.

.. data:: image.UPCA

   UPCA barcode type enum.

.. data:: image.EAN13

   EAN13 barcode type enum.

.. data:: image.ISBN13

   ISBN13 barcode type enum.

.. data:: image.I25

   I25 barcode type enum.

.. data:: image.DATABAR

   DATABAR barcode type enum.

.. data:: image.DATABAR_EXP

   DATABAR_EXP barcode type enum.

.. data:: image.CODABAR

   CODABAR barcode type enum.

.. data:: image.CODE39

   CODE39 barcode type enum.

.. data:: image.PDF417

   PDF417 barcode type enum - Future (e.g. doesn't work right now).

.. data:: image.CODE93

   CODE93 barcode type enum.

.. data:: image.CODE128

   CODE128 barcode type enum.
