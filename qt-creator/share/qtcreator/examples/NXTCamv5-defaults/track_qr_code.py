# (C) Copyright Mindsensors.com 2017
#	Commercial use of this software is prohibited.
#	uses OpenMV2
# History
# Name        Date          Comment
# Deepak      06/08/17      Created for code refactor
import cam5procs, time, sensor, colour

def runIt():
    print ("track_qr_code")
    cam5procs.logLine("track_qr_code")
    clock = time.clock()
    framecount = 0
    ledState = 0
    ledCounter = 0
    while cam5procs.receive_packet() == 0:
        ledCounter += 1
        framecount += 1
        if ((ledCounter % 5) == 0 ):
            if ( ledState == 0 ):
                ledState = 1
                cam5procs.ledShowColour([0,0,255])
            else:
                ledState = 0
                cam5procs.ledShowColour([0,0,0])
        clock.tick()
        code = "TBD!!"
        tracked = [framecount&0xff,0, ord(code[0]),ord(code[1]),ord(code[2]),ord(code[3]),ord(code[4])]
        cam5procs.send_packet(tracked,7, cam5procs.TRK_BLOB)
        #print (tracked)
        #if<
    #while<

    return True


def runIt_00():
    print ("track_qr_code")
    cam5procs.logLine("track_qr_code")
    clock = time.clock()
    framecount = 0
    ledState = 0
    ledCounter = 0
    sensor.reset()
    sensor.set_pixformat(sensor.GRAYSCALE)
    sensor.set_framesize(sensor.VGA)
    sensor.set_windowing((240, 240)) # look at center 240x240 pixels of the VGA resolution.
    sensor.skip_frames(30)
    sensor.set_auto_gain(False) # must turn this off to prevent image washout...

    while cam5procs.receive_packet() == 0:
        ledCounter += 1
        if ((ledCounter % 5) == 0 ):
            if ( ledState == 0 ):
                ledState = 1
                cam5procs.ledShowColour([0,0,255])
            else:
                ledState = 0
                cam5procs.ledShowColour([0,0,0])
        clock.tick()
        img = sensor.snapshot()
        if ( img != None ):
            framecount +=1
            for code in img.find_qrcodes():
                tracked = [framecount&0xff,0, ord(code[4][0]),ord(code[4][1]),ord(code[4][2]),ord(code[4][3]),ord(code[4][4])]
                cam5procs.send_packet(tracked,7, cam5procs.TRK_QRCODE)
                img.draw_string(10, 10, code[4], 0)
            #for<
        #if<
    #while<

    return True

