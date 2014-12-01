# pebblue

Source code demonstrating issues surrounding real time communication over Bluetooth from a Pebble smartwatch through the [**AppMessage**](https://developer.getpebble.com/docs/c/group___app_message.html) API to an iOS-based host device.

Tested on:

* Pebble running Pebble Firmware 2.8
* iPad 2 running iOS 8.1.1

Issues are reproducible on earlier Pebble firmware/iOS combinations as well.

## The issue

According to the Pebble SDK, the use of `app_message_outbox_send()` to send a small piece of data from the Pebble to a host device can at times either fail or be delayed for unspecified reasons and may need to be retried. This presents challenges in trying to transfer data like accelerometer state in real time to the host device.

Ultimately, the use of `app_message_outbox_send()` to send a tuplet to an iOS app is delayed every 5-6 seconds, even when using `app_comm_set_sniff_interval()` to prioritize Bluetooth communications via `SNIFF_INTERVAL_REDUCED`.

## Running the code and observing delays

Fire up the Pebble app on your iOS device. From the Developer menu, locate the IP address of your iOS device and set an environment variable in your terminal session:

    export PEBBLE_IP=192.168.1.31
    
Set up the deploy script:

    chmod +x deploy.sh
    
Execute the script to deploy the app to your Pebble.

    ./deploy.sh
    
Then, execute the [**pebblue-ios**](https://github.com/myktra/pebblue-ios) app in Xcode and watch the debugger for output timings. Use the UP/DOWN buttons on the Pebble to adjust the update speed from 50 Hz to 0.2 Hz. Notice every 5-6 seconds delays will occur between received messages (see samples 21 and 22 below).

    2014-11-30 21:24:10.798 Pebblue[198:15483] [Pebble D9B5] 0.500051 received message {
      0 = 18;
    }
    2014-11-30 21:24:11.298 Pebblue[198:15483] [Pebble D9B5] 0.500139 received message {
      0 = 19;
    }
    2014-11-30 21:24:11.798 Pebblue[198:15483] [Pebble D9B5] 0.499678 received message {
      0 = 20;
    }
    2014-11-30 21:24:12.520 Pebblue[198:15483] [Pebble D9B5] 0.722223 received message {
      0 = 21;
    }
    2014-11-30 21:24:14.061 Pebblue[198:15483] [Pebble D9B5] 1.541436 received message {
      0 = 22;
    }
    2014-11-30 21:24:14.563 Pebblue[198:15483] [Pebble D9B5] 0.501662 received message {
      0 = 23;
    }
    2014-11-30 21:24:15.062 Pebblue[198:15483] [Pebble D9B5] 0.499549 received message {
      0 = 24;
    }
