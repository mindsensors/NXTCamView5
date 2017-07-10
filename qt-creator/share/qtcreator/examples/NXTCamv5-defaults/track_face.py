# (C) Copyright Mindsensors.com 2017
#	Commercial use of this software is prohibited.
#	uses OpenMV2
# History
# Name        Date          Comment
# Deepak      06/08/17      Created for code refactor
import cam5procs, time, sensor, colour, image

def runIt ():
    print ("track_face");
    cam5procs.logLine("track_face")
    clock = time.clock()
    ledState = 0
    ledCounter = 0
    framecount = 0
    sensor.set_framesize(sensor.HQVGA)
    sensor.set_pixformat(sensor.GRAYSCALE)
    face_cascade = image.HaarCascade("frontalface", stages=25)
    print(face_cascade)
    while not cam5procs.receive_packet():
        ledCounter += 1
        if ((ledCounter % 5) == 0 ):
            if ( ledState == 0 ):
                ledState = 1
                cam5procs.ledShowColour([0,0,255])
            else:
                ledState = 0
                cam5procs.ledShowColour([0,0,0])
        clock.tick()
        img = sensor.snapshot() # Capture snapshot
        # Find faces.
        # Note: Lower scale factor scales-down the image more and detects smaller objects.
        # Higher threshold results in a higher detection rate, with more false positives.
        faces = img.find_features(face_cascade, threshold=0.75, scale=1.35)
        # Draw objects
        framecount +=1
        for r in faces[:8]:
            img.draw_rectangle(r)
            tracked = [framecount&0xff,0, 0,r[0],r[1],r[0]+r[2],r[1]+r[3]]
            cam5procs.send_packet(tracked,7, cam5procs.TRK_BLOB)
    # Print FPS.
    # Note: Actual FPS is higher, streaming the FB makes it slower.
    return True


