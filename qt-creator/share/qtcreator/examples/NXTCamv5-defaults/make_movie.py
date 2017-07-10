# (C) Copyright Mindsensors.com 2017
#	Commercial use of this software is prohibited.
#	uses OpenMV2
# History
# Name        Date          Comment
# Deepak      06/08/17      Created for code refactor
import cam5procs, sensor, time, mjpeg

def runIt ():
    print ("make_movie")
    cam5procs.logLine("make_movie")
    cam5procs.rcv_type = cam5procs.old_rcv_type
    sensor.reset() # Initialize the camera sensor.
    sensor.set_pixformat(sensor.RGB565)
    sensor.set_framesize(sensor.QVGA)
    sensor.skip_frames(10) # Let new settings take affect.
    clock = time.clock() # Tracks FPS.

    cam5procs.ledShowColour([0, 255, 0])
    sensor.skip_frames(30) # Give the user time to get ready.

    cam5procs.ledShowColour([0, 0, 0])
    cam5procs.ledShowColour([0, 255, 0])

    try:
        m = mjpeg.Mjpeg(cam5procs.fileNamePrefix%(cam5procs.fileNameCounter)+".mjpeg", loop=True)
        cam5procs.fileNameCounter += 1
    except:
        cam5procs.logLine("Error creating movie file")
        return 1

    for i in range(75):
        if (cam5procs.receive_packet() == 0):
            clock.tick()
            m.add_frame(sensor.snapshot())
        else:
            m.close(clock.fps())
            cam5procs.ledShowColour([0, 0, 0])
            return 1
        #print(clock.fps())

    m.close(clock.fps())
    cam5procs.ledShowColour([0, 0, 0])
    return 0
