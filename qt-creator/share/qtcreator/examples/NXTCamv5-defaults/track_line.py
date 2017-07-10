# (C) Copyright Mindsensors.com 2017
#	Commercial use of this software is prohibited.
#	uses OpenMV2
# History
# Name        Date          Comment
# Deepak      06/08/17      Created for code refactor
import cam5procs, time, sensor, colour

def runIt ():
    framecount = 0
    print ("track_line");
    cam5procs.logLine("tracking line")
    ledState = 0
    ledCounter = 0
    sensor.reset()
    sensor.set_pixformat(sensor.GRAYSCALE)
    sensor.set_framesize(sensor.HQVGA)
    sensor.skip_frames(30)
    sensor.set_auto_gain(False)
    sensor.set_auto_whitebal(False)
    GRAYSCALE_THRESHOLD = [(0, 64)]
    ROIS = [(0, i*15, 250, 15) for i in range(8)]

    while not cam5procs.receive_packet():
        ledCounter += 1
        if ((ledCounter % 5) == 0 ):
            if ( ledState == 0 ):
                ledState = 1
                cam5procs.ledShowColour([0,0,255])
            else:
                ledState = 0
                cam5procs.ledShowColour([0,0,0])
        img = sensor.snapshot()
        if ( img != None ):
            framecount +=1
            for i,r in enumerate(ROIS):
                blobs = img.find_blobs(GRAYSCALE_THRESHOLD, roi=r, merge=True)
                if blobs:
                    largest_blob = max(blobs, key=lambda b: b.pixels())
                    tracked = [framecount&0xff,0, i,largest_blob.rect()[0],largest_blob.rect()[1],largest_blob.rect()[0]+largest_blob.rect()[2],largest_blob.rect()[1]+largest_blob.rect()[3]]
                    cam5procs.send_packet(tracked, 7, cam5procs.TRK_BLOB)
                    img.draw_rectangle(largest_blob.rect())
                    img.draw_cross(largest_blob.cx(), largest_blob.cy())

