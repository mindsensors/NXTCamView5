.. _quickref:

Quick reference for the openmvcam
=================================

OpenMV Cam
----------

.. image:: pinout.png
    :alt: OpenMV Cam pinout
    :width: 700px

General board control
---------------------

See :mod:`pyb`. ::

    import pyb

    pyb.delay(50) # wait 50 milliseconds
    pyb.millis() # number of milliseconds since bootup
    pyb.repl_uart(pyb.UART(3, 9600)) # duplicate REPL on UART(3)
    pyb.wfi() # pause CPU, waiting for interrupt
    pyb.stop() # stop CPU, waiting for external interrupt

LEDs
----

See :ref:`pyb.LED <pyb.LED>`. ::

    from pyb import LED

    led = LED(1) # red led
    led.toggle()
    led.on()
    led.off()

Here's the LED Pinout:

* LED(1) -> Red LED
* LED(2) -> Green LED
* LED(3) -> Blue LED
* LED(4) -> IR LEDs

Pins and GPIO
-------------

See :ref:`pyb.Pin <pyb.Pin>`. ::

    from pyb import Pin

    p_out = Pin('P7', Pin.OUT_PP)
    p_out.high()
    p_out.low()

    p_in = Pin('P7', Pin.IN, Pin.PULL_UP)
    p_in.value() # get value, 0 or 1

Here's the GPIO Pinout:

* Pin('P0') -> P0 (PB15) - 5V Tolerant w/ 3.3V output at up to 25 mA drive.
* Pin('P1') -> P1 (PB14) - 5V Tolerant w/ 3.3V output at up to 25 mA drive.
* Pin('P2') -> P2 (PB13) - 5V Tolerant w/ 3.3V output at up to 25 mA drive.
* Pin('P3') -> P3 (PB12) - 5V Tolerant w/ 3.3V output at up to 25 mA drive.
* Pin('P4') -> P4 (PB10) - 5V Tolerant w/ 3.3V output at up to 25 mA drive.
* Pin('P5') -> P5 (PB11) - 5V Tolerant w/ 3.3V output at up to 25 mA drive.
* Pin('P6') -> P6 (PA5)  - 5V Tolerant w/ 3.3V output at up to 25 mA drive.
* Pin('P7') -> P7 (PD12) - 5V Tolerant w/ 3.3V output at up to 25 mA drive.
* Pin('P8') -> P8 (PD13) - 5V Tolerant w/ 3.3V output at up to 25 mA drive.

Only on the OpenMV Cam M7:

* Pin('P9') -> P9 (PD14) - 5V Tolerant w/ 3.3V output at up to 25 mA drive.

Do not draw more than 120 mA in total across all I/O pins.

Servo control
-------------

See :ref:`pyb.Servo <pyb.Servo>`. ::

    from pyb import Servo

    s1 = Servo(1) # servo on position 1 (P7)
    s1.angle(45) # move to 45 degrees
    s1.angle(-60, 1500) # move to -60 degrees in 1500ms
    s1.speed(50) # for continuous rotation servos

Here's the Servo Pinout:

* Servo(1) -> P7 (PD12)
* Servo(2) -> P8 (PD13)

Only on the OpenMV Cam M7:

* Servo(3) -> P9 (PD14)

External interrupts
-------------------

See :ref:`pyb.ExtInt <pyb.ExtInt>`. ::

    from pyb import Pin, ExtInt

    callback = lambda e: print("intr")
    ext = ExtInt(Pin('P7'), ExtInt.IRQ_RISING, Pin.PULL_NONE, callback)

Here's the GPIO Pinout:

* Pin('P0') -> P0 (PB15)
* Pin('P1') -> P1 (PB14)
* Pin('P2') -> P2 (PB13)
* Pin('P3') -> P3 (PB12)
* Pin('P4') -> P4 (PB10)
* Pin('P5') -> P5 (PB11)
* Pin('P6') -> P6 (PA5)
* Pin('P7') -> P7 (PD12)
* Pin('P8') -> P8 (PD13)

Only on the OpenMV Cam M7:

* Pin('P9') -> P9 (PD14)

Timers
------

See :ref:`pyb.Timer <pyb.Timer>`. ::

    from pyb import Timer

    tim = Timer(4, freq=1000)
    tim.counter() # get counter value
    tim.freq(0.5) # 0.5 Hz
    tim.callback(lambda t: pyb.LED(1).toggle())

Here's the Timer Pinout:

* Timer 1 Channel 3 Negative -> P0
* Timer 1 Channel 2 Negative -> P1
* Timer 1 Channel 1 Negative -> P2
* Timer 2 Channel 3 Positive -> P4
* Timer 2 Channel 4 Positive -> P5
* Timer 2 Channel 1 Positive -> P6
* Timer 4 Channel 1 Negative -> P7
* Timer 4 Channel 2 Negative -> P8

Only on the OpenMV Cam M7:

* Timer 4 Channel 3 Positive -> P9

PWM (pulse width modulation)
----------------------------

See :ref:`pyb.Pin <pyb.Pin>` and :ref:`pyb.Timer <pyb.Timer>`. ::

    from pyb import Pin, Timer

    p = Pin('P7') # P7 has TIM4, CH1
    tim = Timer(4, freq=1000)
    ch = tim.channel(1, Timer.PWM, pin=p)
    ch.pulse_width_percent(50)

Here's the Timer Pinout:

* Timer 1 Channel 3 Negative -> P0
* Timer 1 Channel 2 Negative -> P1
* Timer 1 Channel 1 Negative -> P2
* Timer 2 Channel 3 Positive -> P4
* Timer 2 Channel 4 Positive -> P5
* Timer 2 Channel 1 Positive -> P6
* Timer 4 Channel 1 Negative -> P7
* Timer 4 Channel 2 Negative -> P8

Only on the OpenMV Cam M7:

* Timer 4 Channel 3 Positive -> P9

ADC (analog to digital conversion)
----------------------------------

See :ref:`pyb.Pin <pyb.Pin>` and :ref:`pyb.ADC <pyb.ADC>`. ::

    from pyb import Pin, ADC

    adc = ADC('P6')
    adc.read() # read value, 0-4095

Here's the ADC Pinout:

* ADC('P6') -> P6 (PA5) - ONLY 3.3V (NOT 5V) TOLERANT IN THIS MODE!

DAC (digital to analog conversion)
----------------------------------

See :ref:`pyb.Pin <pyb.Pin>` and :ref:`pyb.DAC <pyb.DAC>`. ::

    from pyb import Pin, DAC

    dac = DAC('P6')
    dac.write(120) # output between 0 and 255

Here's the ADC Pinout:

* DAC('P6') -> P6 (PA5) - ONLY 3.3V (NOT 5V) TOLERANT IN THIS MODE!

UART (serial bus)
-----------------

See :ref:`pyb.UART <pyb.UART>`. ::

    from pyb import UART

    uart = UART(3, 9600)
    uart.write('hello')
    uart.read(5) # read up to 5 bytes

Here's the UART Pinout:

* UART 3 RX -> P5 (PB11)
* UART 3 TX -> P4 (PB10)

Only on the OpenMV Cam M7:

* UART 1 RX -> P0 (PB15)
* UART 1 TX -> P1 (PB14)

SPI bus
-------

See :ref:`pyb.SPI <pyb.SPI>`. ::

    from pyb import SPI

    spi = SPI(2, SPI.MASTER, baudrate=200000, polarity=1, phase=0)
    spi.send('hello')
    spi.recv(5) # receive 5 bytes on the bus
    spi.send_recv('hello') # send a receive 5 bytes

Here's the UART Pinout:

* SPI 2 MOSI (Master-Out-Slave-In) -> P0 (PB15)
* SPI 2 MISO (Master-In-Slave-Out) -> P1 (PB14)
* SPI 2 SCLK (Serial Clock)        -> P2 (PB13)
* SPI 2 SS   (Serial Select)       -> P3 (PB12)

I2C bus
-------

See :ref:`pyb.I2C <pyb.I2C>`. ::

    from pyb import I2C

    i2c = I2C(2, I2C.MASTER, baudrate=100000)
    i2c.scan() # returns list of slave addresses
    i2c.send('hello', 0x42) # send 5 bytes to slave with address 0x42
    i2c.recv(5, 0x42) # receive 5 bytes from slave
    i2c.mem_read(2, 0x42, 0x10) # read 2 bytes from slave 0x42, slave memory 0x10
    i2c.mem_write('xy', 0x42, 0x10) # write 2 bytes to slave 0x42, slave memory 0x10

Here's the I2C Pinout:

* I2C 2 SCL (Serial Clock) -> P4 (PB10)
* I2C 2 SDA (Serial Data)  -> P5 (PB11)

Only on the OpenMV Cam M7:

* I2C 4 SCL (Serial Clock) -> P7 (PD13)
* I2C 4 SDA (Serial Data)  -> P8 (PD12)
