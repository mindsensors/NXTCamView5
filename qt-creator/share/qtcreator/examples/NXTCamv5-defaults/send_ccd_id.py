# (C) Copyright Mindsensors.com 2017
#	Commercial use of this software is prohibited.
#	uses OpenMV2
# History
# Name        Date          Comment
# Deepak      06/08/17      Created for code refactor
import cam5procs, sensor

def runIt ():
    #print ("send_ccd_id");
    l = list()
    l.append(sensor.get_id())
    cam5procs.send_packet(l,len(l),CCDID)


