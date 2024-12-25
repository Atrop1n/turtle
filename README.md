# light_bulb_control
## Control two lightbulbs via a web interface!

This is an automated light and heating control for a turtle tank.

Turles living indoors usually require some sort of artificial lighting and heating. There is a variety of suitable devices on the market.
The one I had consisted of two lightbulbs, one for heating, the other for lighting. Each was supplied with a dedicated manual switch. 

However I decided to make the switching automated instead of having to manually press the switch every morning and evening. 

The two requirements I was keeping in mind while working on the project were reliability and *user-friendliness*.
## Principle
The control system has two modes: auto and manual. In automatic mode, lamps are automatically being switched based on current time and morning and evening timers.
The lamps are off early in the morning, active during the day, and off again in the evening.

![image](https://github.com/user-attachments/assets/be389212-3ef2-4d0a-a628-c448ea1ecb3b | width=100)

In manual mode, both heating and light outputs are directly controlled by the user.

![image](https://github.com/user-attachments/assets/270e7dd9-0260-40d4-af66-ae4b36ed5d8e)

That's it. This project is pretty simple.

## Hardware
```
1x DOIT DevKit V1 ESP32
2x relay
1x LM2596 Step-down converter
1x 24 V power supply
3D printer
some cables and WAGOs
```
The core of the system consists of 2 relays which are controlled by an ESP32 microcontroller. Lamps are connected to the load side of the relays. These components are powered by a 24V power supply and a step-down converter regulating the DC voltage further to 5V. 
<img width="684" alt="Schema" src="https://github.com/user-attachments/assets/ce4d0d07-bd09-4afa-8480-f0c328e9bfa4" />
You might ask yourself why didn't I use power supply with 5V output, and you're right. I am using 24V power supply and regulating is down to 5 V solely becuase 
I did not have any 5V power supply available at the time.

The project required a custom-made housing which I designed and printed.  
First I had to decide the overall size of the housing so that it fits the turtle tank and contains all the components.
Next I chose appropriate position for every component in the housing and made a screw socket which copied position of every mounting hole
on the individual components. A general rule of thumb is to place components that are related to each other in close vicinity. 
I proceeded by adding walls and lid mouting platforms in each corner. I finished the design process by creating the lid itself. 

![image](https://github.com/user-attachments/assets/bf974932-0d91-4acb-861e-61319a380f80)

After the plastic parts were printed out, I mounted all the components to the housing using screws. I also removed the original cable from the lamps and connected the lamps to the relay outputs.  
When placing cables, it is crucial to separate power (220V) and signal (5V) lines as much as possible. They must not be sharing the same Wago. Also they should not touch each other otherwise noise will be introduced, causing all sorts of unwanted effects. 

![components_inside](https://github.com/user-attachments/assets/08503f9b-df52-4f25-9a49-74ed1ba88a73)
*Power AC lines on the right side, signal DC lines on the left.* 

When the hardware was ready, I started writing the code. 
![image](https://github.com/user-attachments/assets/6b8ace03-875f-48c7-b32b-992e144bc822)
In the main loop current time is periodically being updated and compared to start and end times of the light and heating.
<img width="684" alt="diagram_auto" src="https://github.com/user-attachments/assets/dc6fde82-a58e-4dd0-a1df-22432d13fe7f" />
When in manual mode, bulbs are controlled by webpage switches instead.
<img width="684" alt="diagram_manual" src="https://github.com/user-attachments/assets/0b6a5466-6e5c-4291-a639-5cceaddfd4eb" />
Switch states are being retrieved by http methods.
Detailed code explanations can be found in the code itself.

After the code was flashed (and all bugs were eliminated) I now had my own lamp controlling system!
![finished](https://github.com/user-attachments/assets/5df67ec7-a814-4e28-8521-575a39b0e20c)
*Very happy turtle (can you spot her?)* 


                                                                  
