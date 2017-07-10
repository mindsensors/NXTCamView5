# (C) Copyright Mindsensors.com 2017
#	Commercial use of this software is prohibited.
#	uses OpenMV2
# History
# Name        Date          Comment
# Deepak      06/08/17      Created for code refactor
import cam5procs, time, sensor, colour, image

def runIt ():
    print ("track_eye");
    cam5procs.logLine("track_eye")
    clock = time.clock()
    framecount = 0
    ledState = 0
    ledCounter = 0
    sensor.set_framesize(sensor.HQVGA)
    sensor.set_pixformat(sensor.GRAYSCALE)
    face_cascade = image.HaarCascade("frontalface", stages=25)
    eyes_cascade = image.HaarCascade("eye", stages=24)
    #print(face_cascade,eyes_cascade)
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
        for face in faces[:4]:
            img.draw_rectangle(face)
            # Now find eyes within each face.
            # Note: Use a higher threshold here (more detections) and lower scale (to find small objects)
            eyes = img.find_features(eyes_cascade, threshold=0.5, scale=1.2, roi=face)
            for e in eyes[:2]:
                img.draw_rectangle(e)
                tracked = [framecount&0xff,0, 0,e[0],e[1],e[0]+e[2],e[1]+e[3]]
                #print("fps: ", clock.fps(),tracked)
                cam5procs.send_packet(tracked,7, cam5procs.TRK_BLOB)
    # Print FPS.
    # Note: Actual FPS is higher, streaming the FB makes it slower.
    return True


