# (C) Copyright Mindsensors.com 2017
#	Commercial use of this software is prohibited.
#	uses OpenMV2
# History
# Name        Date          Comment
# Deepak      06/08/17      Created for code refactor
import cam5procs, sensor, time, mjpeg, image

def runIt ():
    print ("still_picture");
    cam5procs.logLine("capturing a still picture")
    error_code = 0
    sensor.reset() # Initialize the camera sensor.
    sensor.set_pixformat(sensor.RGB565) # or sensor.GRAYSCALE
    sensor.set_framesize(sensor.QVGA) # or sensor.QQVGA (or others)
    sensor.skip_frames(10) # Let new settings take affect.
    cam5procs.rcv_type = cam5procs.old_rcv_type  # go back to previous action when done
    cam5procs.ledShowColour([0,255,0])
    try:
        img = sensor.snapshot()
        img.save(cam5procs.fileNamePrefix%(cam5procs.fileNameCounter)+".jpg")
        cam5procs.fileNameCounter += 1
    except:
        error_code = 1
        cam5procs.ledShowColour([255,0,0])

    cam5procs.ledShowColour([0,0,0])

