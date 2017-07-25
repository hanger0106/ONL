"""
Test case 1: Set rpm to 15000
"""
DEBUG = True #Static constant used for debugging
user_rpm = 15000

from libFan import fan
from libFan import get_fans
from time import sleep

fanobj = get_fans() #Get all the fans and their statuses
count = len(fanobj) # Count the number of fans
print "The count is: ",count

for y in range(count):
    fan.set_normal_speed(fanobj[y]) #Set speed to 8000 rpm
sleep(5)


rpmlist = [] #list to store all the fans' rpm
for x in range(count):
    rpmold = fan.get_rpm(fanobj[x]) #rpm before setting to user_rpm
    rpmNew = fan.set_rpm(fanobj[x],user_rpm)
    if(rpmNew != None):
        rpmlist.append(rpmNew)

if DEBUG:
    print "After setting credentials"
    print rpmlist

    # Pass percentage
    user_rpm_lower = user_rpm - 300
    user_rpm_higher = user_rpm + 300

    fail = 0
    pass1 = 0

    for i in range(len(rpmlist)):
        if user_rpm_lower <= rpmlist[i] <= user_rpm_higher:
            pass1 = pass1 + 1
        else:
            fail = fail + 1

    if fail != 0:
        final = "fail"
        print "Test case failed"
    else:
        final = "pass"
        print "Test case passed"
