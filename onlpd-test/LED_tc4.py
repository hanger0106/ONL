"""
Test case 4: Turn led[1] to state 0(OFF)
"""
user_state = 0
DEBUG = True

from libonlp import led
from libonlp import get_leds
from time import sleep

ledobj = get_leds()
count = len(ledobj)
print "The count is : ",count

for x in range(count):
    led.set_normal(ledobj[x])

led.set_state(ledobj[1],user_state)
sleep(3)
currentState = led.get_state(ledobj[1])

if DEBUG:
    if currentState == user_state:
        print "Test case passed"

    elif currentState == 'None':
        print "Check the LED capabilities"

    else:
        print "Test case failed"
