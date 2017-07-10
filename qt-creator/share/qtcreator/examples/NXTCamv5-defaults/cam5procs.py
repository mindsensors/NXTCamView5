# (C) Copyright Mindsensors.com 2017
#	Commercial use of this software is prohibited.
#	uses OpenMV2
# History
# Name        Date          Comment
# Deepak      06/08/17      Created for code refactor

import sensor, image, time, mjpeg, math, gif
import pyb, sys
from pyb import UART, Pin, Timer
import colour

# defines used in this program
MODEL = 0
ENGNVER = 1
CCDID  = 2
CMDPKT = 3
TRK_BLOB = 4
TRK_LINE = 5
STILLIMAGE = 6
MOVIE = 7
MOVIER = 8
TRK_FACE  = 9
TRK_EYE  = 10
TRK_QRCODE  = 11
COLOURMAP = 50

session_id = str( pyb.millis() )
fileNamePrefix = 'M'+session_id+"_%.2d"
fileNameCounter = 0

RedPin = Pin('D12')
GreenPin = Pin('D13')
BluePin = Pin('D14')
tim = Timer(4, freq=10000)
Red = tim.channel(1, Timer.PWM, pin=RedPin)
Green = tim.channel(2, Timer.PWM, pin=GreenPin)
Blue = tim.channel(3, Timer.PWM, pin=BluePin)
Red.pulse_width_percent(100)
Green.pulse_width_percent(100)
Blue.pulse_width_percent(100)


rcv_len = 0
rcv_type = 0
old_rcv_type = 0
rcv_chksum = 0
rcv_count = 0
MVstate = 0
rcv_data = [0 for x in range(32)]

uart = UART(3, 115200)

def logLine(msg):
   global session_id
   log_file = open ("NXTCam5_log.txt", "a") 
   log_file.write( session_id + ":" + msg +"\n")
   log_file.close()

def receive_packet():
    global MVstate, rcv_len,rcv_type,rcv_data,rcv_chksum,rcv_count,old_rcv_type

    for rcv_char in uart.read(uart.any()) :

        if MVstate == 0:
            if rcv_char ==0x55 :MVstate = 1
        elif  MVstate == 1:
            if rcv_char ==0xaa : MVstate = 2
            else: MVstate = 0
        elif  MVstate == 2:
            rcv_len =rcv_char
            rcv_chksum = 0xff+ rcv_len
            rcv_count = 0
            MVstate = 3
        elif  MVstate == 3:
            old_rcv_type = rcv_type
            rcv_type =rcv_char
            rcv_chksum +=  rcv_type
            MVstate = 4
        elif  MVstate == 4:
            rcv_data[rcv_count] =rcv_char
            rcv_chksum += rcv_data[rcv_count]
            rcv_count +=1
            if rcv_count>= rcv_len :MVstate = 5
        elif  MVstate == 5:
            MVstate = 0
            if rcv_char&0xff == rcv_chksum&0xff :return rcv_len
            else :return 0
    return 0

def ledShowColour( ledcolour=(0,0,0)):
    #print(255-ledcolour[0],255-ledcolour[1],255-ledcolour[2])
    Red.pulse_width_percent((255-ledcolour[0])/2.55)
    Green.pulse_width_percent((255-ledcolour[1])/2.55)
    Blue.pulse_width_percent((255-ledcolour[2])/2.55)

def send_packet(sendbuff,length = 7,packettype =0):
    #print ("sendbuff: ", sendbuff)
    chksum = 0
    uart.writechar(0x55)
    #pyb.delay(1)
    uart.writechar(0xaa)
    #pyb.delay(1)
    chksum = 0x55+0xaa+length+packettype
    uart.writechar(length)
    #pyb.delay(1)
    uart.writechar(packettype)
    for i in range(0, length):
        #pyb.delay(1)
        uart.writechar(sendbuff[i])
        chksum = chksum + sendbuff[i]
    uart.writechar((chksum) )

