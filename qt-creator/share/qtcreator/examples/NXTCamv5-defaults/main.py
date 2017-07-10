# Detection and tracking functions for NXTcam5
# (C) Copyright Mindsensors.com 2017
#	Commercial use of this software is prohibited.
#	uses OpenMV2
# History
# Name        Date          Comment
# Nitin Patil Feb 19 2017   started
# Deepak      06/01/17      multiple function handlers.
# Deepak      06/07/17      Logging function added (logging to NXTCam5_log.txt).

import sensor, image, time, mjpeg, math, gif
import pyb, sys
from pyb import UART, Pin, Timer
import colour

import cam5procs

try:
    nxtcf
except NameError:
    #nxtcf is not defined, so define it to be 'blob tracking'
    nxtcf = cam5procs.TRK_BLOB

sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QVGA)
sensor.skip_frames(30)
sensor.set_auto_gain(False) # must be turned off for color tracking
sensor.set_auto_whitebal(False) # must be turned off for color tracking
clock = time.clock()

funcList = {0 : 'send_model',
    1: 'send_ver',
    2: 'send_ccd_id',
    3: 'send_cmd_packet',
    4: 'track_blob',
    5: 'track_line',
    6: 'still_picture',
    7: 'make_movie',
    8: 'make_con_movie',
    9: 'track_face',
    10: 'track_eye',
    11: 'track_qr_code',
}

cam5procs.logLine("new session created: " + cam5procs.session_id)

framecount = 0
l = len(funcList)
cam5procs.rcv_type = nxtcf
print (cam5procs.rcv_type)

while(True):
    if ( cam5procs.rcv_type > 0 and cam5procs.rcv_type < l):
        mod =  __import__(funcList[cam5procs.rcv_type])
        #print ("received: " + str(funcList[cam5procs.rcv_type]))
        mod.runIt()
    else:
        if (cam5procs.rcv_type >= cam5procs.COLOURMAP):
            #index = rcv_type - COLOURMAP
            #print ("updating colourmap")
            #colourmap[index] = (rcv_data[0],rcv_data[1],rcv_data[2],rcv_data[3],rcv_data[4],rcv_data[5])
            pass
        #if<
    #else<
#while<

