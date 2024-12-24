# light_bulb_control
Control two lightbulbs via a web interface

This is an automated light and heating control for a turtle tank.

Turles living indoors usually require some sort of artificial lighting and heating. There is a variety of suitable devices on the market.
The one I had consisted of two lightbulbs, one for heating, the other for lighting. Each had dedicated manual switch. 
I have decided to make the switching automated, instead of having to manually press the switch every morning and evening.

## Hardware requirements
```
1x DOIT DevKit V1 ESP32
2x relay
1x LM2596 Step-down converter
1x 24 V power supply
3D printer
some cables and WAGOs
```
I started out by designing the housing which I later printed. The design was done in Fusion 360.  

First I had to decide the overall size of the housing so that it fits the turtle tank and contains all the components.
Next I chose appropriate position for every component in the housing and made a 5mm high screw socket which copied position of every mounting hole
on the individual components. A general rule of thumb is to place components that are related to each other in close vicinity. 
I proceeded by adding walls and lid mouting platforms in each corner. I finished the design process by creating the lid itself. 

![image](https://github.com/user-attachments/assets/bf974932-0d91-4acb-861e-61319a380f80)

Then the plastic parts were printed out. At this point I mounted all the components to the housing using screws. I also removed the original cable from the lamp and connected the lamp to the relay outputs.  
Here it is crucial to separate power (220V) and signal (5V) lines as much as possible. They must not be sharing the same Wago. Also they should not touch each other otherwise noise will be introduced, causing all sorts of unwanted effects. 

![components_inside](https://github.com/user-attachments/assets/08503f9b-df52-4f25-9a49-74ed1ba88a73)

When the hardware was ready, I started writing the code. 


                                                                  
