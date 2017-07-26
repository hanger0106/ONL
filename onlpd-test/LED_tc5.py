"""
Test case 5: Turn led[1] to Mode 14(Yellow)
"""
user_mode = 14
DEBUG = True

from libonlp import led
from libonlp import get_leds
from time import sleep

ledobj = get_leds()
count = len(ledobj)
print "The count is : ",count

for x in range(count):
    led.set_normal(ledobj[x]) #Set state to 1 and mode to GREEN

led.set_mode(ledobj[1],user_mode)
sleep(3)
currentState = led.get_mode(ledobj[1])

if DEBUG:
    if currentState == user_mode:
        print "Test case passed"

    elif currentState == 'None':
        print "Check the LED capabilities"

    else:
        print "Test case failed"
