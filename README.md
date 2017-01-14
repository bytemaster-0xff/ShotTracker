# ShotTracker
Windows 10 UWP Application using OpenCV to track holes in targets made by projectiles in real time.

## Notes
The image recognition is not done locally in the app, this a connection to the internet will be required.
A library exists that allows us to call OpenCV functions locally in a UWP app, however the price at this state of the project is prohibative
Since it's not really important to track the shots in millisecond real time, we take an image locally, then upload it to an Azure based service to decode the shots.  The result will be sent back to the device where they can be rendered locally.


