# (C) Copyright Mindsensors.com 2017
#	Commercial use of this software is prohibited.
#	uses OpenMV2
# History
# Name        Date          Comment
# Deepak      06/08/17      Created for code refactor
import cam5procs, time, sensor, colour

def getKey(item):
    return item[4]

def runIt():
    framecount = 0
    ledState = 0
    ledCounter = 0
    print ("track_blob");
    cam5procs.logLine("tracking blob")
    clock = time.clock()

    lastBlob = None
    clock.tick() # Track elapsed milliseconds between snapshots().
    sensor.reset()
    sensor.set_pixformat(sensor.RGB565)
    sensor.set_framesize(sensor.HQVGA)
    sensor.skip_frames(5)
    sensor.set_auto_whitebal(False)
    while cam5procs.receive_packet() == 0:
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
            blobs = img.find_blobs([colour.map[0],colour.map[1],colour.map[2],colour.map[3], colour.map[4], colour.map[5], colour.map[6], colour.map[7]])
            sorted_blobs = sorted(blobs,key=getKey,reverse=True)
            if (sorted_blobs):
                framecount +=1
                for blob in sorted_blobs[:8]:
                    img.draw_rectangle(blob.rect())
                    img.draw_cross(blob.cx(), blob.cy())
                    tracked = [framecount&0xff,0, blob[8],blob[0],blob[1],blob[0]+blob[2],blob[1]+blob[3]]
                    cam5procs.send_packet(tracked,7,cam5procs.TRK_BLOB)
                #for<
            #if<
        #if<
        cam5procs.ledShowColour([0,0,0])
    #while<

