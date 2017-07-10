:mod:`cpufreq` --- easy cpu frequency control
=============================================

.. module:: cpufreq
   :synopsis: easy cpu frequency control

The ``cpufreq`` module is used for easily controlling the cpu frequency.
In particular, the ``cpufreq`` module allows you to easily underclock or
overclock your OpenMV Cam.

Example usage::

    import cpufreq

    cpufreq.set_frequency(cpufreq.CPUFREQ_216MHZ)

The OpenMV Cam M4's default frequency is 180MHz.
The OpenMV Cam M7's default frequency is 216MHz.

Functions
---------

.. function:: cpufreq.get_frequency()

   Returns a tuple containing:

      * [0] - sysclk: frequency of the CPU (int).
      * [1] - hclk: frequency of the AHB bus, core memory and DMA (int).
      * [2] - pclk1: frequency of the APB1 bus (int).
      * [3] - pclk2: frequency of the APB2 bus (int).

.. function:: cpufreq.set_frequency(freq)

   Changes the cpu frequency. ``freq`` may be one of:

      * cpufreq.CPUFREQ_120MHZ
      * cpufreq.CPUFREQ_144MHZ
      * cpufreq.CPUFREQ_168MHZ
      * cpufreq.CPUFREQ_192MHZ
      * cpufreq.CPUFREQ_216MHZ

Constants
---------

.. data:: cpufreq.CPUFREQ_120MHZ

   Used to set the frequency to 120 MHz.

.. data:: cpufreq.CPUFREQ_144MHZ

   Used to set the frequency to 144 MHz.

.. data:: cpufreq.CPUFREQ_168MHZ

   Used to set the frequency to 168 MHz.

.. data:: cpufreq.CPUFREQ_192MHZ

   Used to set the frequency to 192 MHz.

.. data:: cpufreq.CPUFREQ_216MHZ

   Used to set the frequency to 216 MHz.
