# (C) Copyright Mindsensors.com 2017
#	Commercial use of this software is prohibited.
#	uses OpenMV2
# History
# Name        Date          Comment
# Deepak      06/08/17      Created for code refactor
import cam5procs, sys


def runIt ():
    #print ("send_ver")
    cam5procs.send_packet(sys.version.encode('ascii'),len(sys.version.encode('ascii')),ENGNVER)
