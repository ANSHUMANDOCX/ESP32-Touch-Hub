# ESP32 Touch HUB

Its a ESP32 Hub that uses a 3.5 inch touch screen to control things

# 2026-06-14: Finally Built it 

**Total time spent: 0.2 hours**

Setup all the other dash boards and finally completed the build didnt wait for the #D printer to arrive just added some random wooden pices at the back to amke the buttons accessible 

![IMG_20260614_000914.jpg](https://cdn.hackclub.com/019ec51a-08ca-7708-be6c-e5f2daa14564/IMG_20260614_000914.jpg)![IMG_20260614_000950.jpg](https://cdn.hackclub.com/019ec51a-1be9-7b33-9d7f-2f9ab333db5a/IMG_20260614_000950.jpg)![IMG_20260614_001005.jpg](https://cdn.hackclub.com/019ec51a-284d-72ef-9208-c521d96f8689/IMG_20260614_001005.jpg)![IMG_20260614_001013.jpg](https://cdn.hackclub.com/019ec51a-3840-7fb6-aaca-e6aa1969c834/IMG_20260614_001013.jpg)
![IMG_20260614_001022.jpg](https://cdn.hackclub.com/019ec51c-928d-761d-a081-1b7ffe2a2a01/IMG_20260614_001022.jpg)

demo video- https://youtube.com/shorts/PqJHbwtBgcA

# 2026-06-13: Setup the final Dashboard 

**Total time spent: 0.75 hours**

here is the final dashboard coded it no problems were left apart from a few the only thing now remaining is added spacers for the buttons so i can actually use those but i am waiting on getting my 3dp to get it printed also since my sensor node it not yet ready hence the sensor data is not being updated ![WhatsApp Image 2026-06-13 at 9.23.56 PM.jpeg](https://cdn.hackclub.com/019ec1b1-ce76-7c78-a3e2-ecff931c5bdd/WhatsApp%20Image%202026-06-13%20at%209.23.56%20PM.jpeg)

hackatime project name - ESP32 Touch HUB


# 2026-06-10: Working on the CODE

**Total time spent: 2.5 hours**

Started out by wrting a simple code to scan the wifi around me aand used the buttons to navigate the details all was fine until i tried to upload the code which was a mistake since i rushed into it and faced a lot of problem spent a lot of time debugging it and found out that my even tho my esp32s3 is n16r8 the psram is not working. after that went back to square one where i uploaded code one by one to test each section and move on 

here is the hackatime project name ESP32 Touch HUB

![IMG_20260610_130057.jpg](https://cdn.hackclub.com/019eb071-bd47-7406-9926-689b22a2fd2b/IMG_20260610_130057.jpg)

![IMG_20260610_130056.jpg](https://cdn.hackclub.com/019eb071-c6bf-7219-bb11-4cf2f9aefe94/IMG_20260610_130056.jpg)![IMG_20260610_130054.jpg](https://cdn.hackclub.com/019eb071-deb6-7766-9ba1-fa7769e23d8a/IMG_20260610_130054.jpg)![IMG_20260610_130051.jpg](https://cdn.hackclub.com/019eb071-eaff-72d6-ac38-2f0f6389302b/IMG_20260610_130051.jpg)![IMG_20260610_130047.jpg](https://cdn.hackclub.com/019eb071-f8e9-75e8-826d-29132531f365/IMG_20260610_130047.jpg)![IMG_20260610_130032.jpg](https://cdn.hackclub.com/019eb072-05b7-7d96-add9-f50200aa73e9/IMG_20260610_130032.jpg)![IMG_20260610_130024.jpg](https://cdn.hackclub.com/019eb072-13a9-7d23-b265-fc40aad41bc4/IMG_20260610_130024.jpg)

# 2026-06-03: Diagnosed the LCD

**Total time spent: 2 hours**

So it turns out that the touch on my LCD is faulty and i wont be able to use the touch feature any more at least not unti i get a replacement which could be a while so for now the board will be based on using the buttons i alr added good thing that i did so anyways its a dashboard so i dont think it will be a big prob just makes me sad that i wont be able to make what i intended to

the display is working fine tho just the touch 
![IMG_20260603_213844.jpg](https://cdn.hackclub.com/019e8e47-6315-7364-983d-4004f5302002/IMG_20260603_213844.jpg)
![IMG_20260603_214553.jpg](https://cdn.hackclub.com/019e8e47-755a-725b-bb7d-c85f2b5eaa1d/IMG_20260603_214553.jpg)
![IMG_20260603_214506.jpg](https://cdn.hackclub.com/019e8e47-8dc1-716c-9683-92b8ea5f4973/IMG_20260603_214506.jpg)
this is what i am getting when i try to read the touch data 
![image.png](https://cdn.hackclub.com/019e8e37-9f0f-798d-ad03-a97314737709/image.png)

# 2026-05-31: Completed PCB assembly 

**Total time spent: 5 hours**

So completed the PCB assembly today took a lot of time to completed and also faced some problems along the way 

1. Setting up the PCB for applying the paste 
this was pretty straight forward just laid the pcb i wanted past between 4 other pcbs to make it stay in one place stuck the other pcbs to my table using masking tape 
![IMG_20260530_194012.jpg](https://cdn.hackclub.com/019e7ea2-df46-7e79-a4e6-619f812760a8/IMG_20260530_194012.jpg)

2. completed applying the paste 
the usualy 45 and 90 degree swiping the card thing pretty simple tbh nothing major but made a mistake here accidently used a old paste which was not enough wet and turned out to be a pain in the ass later on
![IMG_20260531_135700.jpg](https://cdn.hackclub.com/019e7ea3-36fd-7e86-a93f-ee077a6578cf/IMG_20260531_135700.jpg)![IMG_20260531_135656.jpg](https://cdn.hackclub.com/019e7ea3-5199-7c9e-bace-2a2b8b89e688/IMG_20260531_135656.jpg)
3. Placing the component
palced all the componets by looking at the ibom from kicad 
![IMG_20260531_150533.jpg](https://cdn.hackclub.com/019e7ea4-8284-7668-81a0-3d0f633fa60b/IMG_20260531_150533.jpg)
3. reflowing the paste 
here is where i faced the problem due the quality of the paste it didnt properly reflowed so even if the solder flowed it didnt make a good joint so it was majorly a blunder 

4. Fixing the Joints
to fix em i had to manually heat every pad with a soldering iron and make sure that the joint was solid 
here is a footage i took from my microscope

https://drive.google.com/file/d/1vjfSiBI4xpelcF_Ak4XN7pS9MMJF1--t/view?usp=sharing
https://drive.google.com/file/d/1b6f4MoLTS4rwQKdUu72TKH13SkypEIQ1/view?usp=sharing

5. Inital testing 
so powered up the the board and found out that nothig blew up yay!! but it turned out the esp32 chip wasnt communicating i couldnt understand why then i took a look at my schematic and it turned out that i can connected the RX and the TX of the wrong but luckily it wasnt a very big thing just a pain in the ass to fix since i wanted it to be clean so i cut the trace and connected it using some very thin wire 
here is the footage to that its limited cuz my memory card ran out of space 
https://drive.google.com/file/d/1RYIBsxv1aN_MEL4bykIt0hbc3sjk9e4Q/view?usp=sharing
https://drive.google.com/file/d/1l4DWNKCJhttoH9iQIVY37mo3bGiGoVLq/view?usp=sharing
https://drive.google.com/file/d/1rmzW9IPVHRcFvySrBTWPCZd45q94Sqj3/view?usp=sharing
https://drive.google.com/file/d/1F8eLT4zi97B3lu4MnuW7eXseRLjP_RLU/view?usp=sharing

![IMG_20260531_175729.jpg](https://cdn.hackclub.com/019e7eae-03aa-7a3c-a913-cc01fae7f1a7/IMG_20260531_175729.jpg)
![IMG_20260531_175733.jpg](https://cdn.hackclub.com/019e7eae-11e0-74ef-acf6-b273759cfbd0/IMG_20260531_175733.jpg)

6. Attaching the LCD and changed the colour of the lights for better indication
Finally after everything was fixed and fine i soldered on the lcd and changed the color of the light for better indication of hte charging status

![IMG_20260531_200409.jpg](https://cdn.hackclub.com/019e7eae-53f6-7007-9d10-515d939e21a6/IMG_20260531_200409.jpg)
![IMG_20260531_200413.jpg](https://cdn.hackclub.com/019e7eae-6bb2-7ac7-9ae0-38986447ecbf/IMG_20260531_200413.jpg)
![IMG_20260531_200415.jpg](https://cdn.hackclub.com/019e7eae-8106-7038-a7f0-935ae084b915/IMG_20260531_200415.jpg)
![IMG_20260531_200434.jpg](https://cdn.hackclub.com/019e7eae-9ec8-7b46-8abd-3f79a1b7cb95/IMG_20260531_200434.jpg)
Now all thats left is the software part of the model 

# 2026-05-24: Enclosure fitments 

**Total time spent: 1 hour**

Worked on fitting the LCD flush into the enclosure it included sanding adding the heatset inserts finishing and finally adding the screws. It just feels so goos to see your measuremts for a 3d model turn out to be great when you dont event have the parts and just make it using a random datasheet and 3d model online 

![IMG_20260524_213134.jpg](https://cdn.hackclub.com/019e5abb-52f4-730f-9c51-613d726d5970/IMG_20260524_213134.jpg)![IMG_20260524_213115.jpg](https://cdn.hackclub.com/019e5abb-601a-722a-8a8c-a420b36a854d/IMG_20260524_213115.jpg)![IMG_20260524_213001.jpg](https://cdn.hackclub.com/019e5abb-6de1-7103-8775-9192a7aed478/IMG_20260524_213001.jpg)

that white residue is due to the acetone i used to cool the inserts turnout to be a bad idea

# 2026-04-27: More cad work

**Total time spent: 2.5 hours**

Made the Back plate of the Case along with adding the holes slots etc for the card type c port and LEDs made a mistake while designing accidently joined all the bodies together lol
![image.png](https://forge.hackclub.com/rails/active_storage/blobs/proxy/eyJfcmFpbHMiOnsiZGF0YSI6MTM1LCJwdXIiOiJibG9iX2lkIn19--a0e54a94f8fba997439a9667e362dc779a6cf9a2/image.png)![image.png](https://forge.hackclub.com/rails/active_storage/blobs/proxy/eyJfcmFpbHMiOnsiZGF0YSI6MTM2LCJwdXIiOiJibG9iX2lkIn19--af4cd77cc795cacf28d02b88b7b304e1113f8ecc/image.png)

# 2026-04-26: CAD work

**Total time spent: 3 hours**

did a majority of the CAD for the HUB making the main enclosure and properly aligned the holes for the dispaly
![image.png](https://forge.hackclub.com/rails/active_storage/blobs/proxy/eyJfcmFpbHMiOnsiZGF0YSI6MTMzLCJwdXIiOiJibG9iX2lkIn19--613cf0a15845d61ff234d345e959cb9bc2496aa6/image.png)

# 2026-04-26: Some changes

**Total time spent: 0.5 hours**

Ran the DRC made some changes got the 3d view had to fix a few part nothing much
![image.png](https://forge.hackclub.com/rails/active_storage/blobs/proxy/eyJfcmFpbHMiOnsiZGF0YSI6MTI3LCJwdXIiOiJibG9iX2lkIn19--0adabbd71f67e4fe8681f68b17732f07e4f29627/image.png)![Screenshot 2026-04-26 224944.png](https://forge.hackclub.com/rails/active_storage/blobs/proxy/eyJfcmFpbHMiOnsiZGF0YSI6MTI4LCJwdXIiOiJibG9iX2lkIn19--3dae260ded6052cdc2ecd313212a5b9b2e6de0de/Screenshot%202026-04-26%20224944.png)

# 2026-04-25: PCB Design Part 2

**Total time spent: 1.5 hours**

Added some markers on The Silk screen along with chaning the footprint of the SD card holder i chose the most common one out there 
![image.png](https://forge.hackclub.com/rails/active_storage/blobs/proxy/eyJfcmFpbHMiOnsiZGF0YSI6ODEsInB1ciI6ImJsb2JfaWQifX0=--3501197c116ca8fcca32d9e33fa719666d99f329/image.png)

# 2026-04-24: Preliminary Routing

**Total time spent: 2.5 hours**

i have completed the preliminary routing of the PCB is done all the part are routed for the first time which i thut were the best path however i will need to  take more look on it make it perfect![image.png](https://forge.hackclub.com/rails/active_storage/blobs/proxy/eyJfcmFpbHMiOnsiZGF0YSI6NzgsInB1ciI6ImJsb2JfaWQifX0=--3b1d41bb8f884b8afa2ade788459e6bd3628bc32/image.png)

# 2026-04-24: PCB Design

**Total time spent: 2 hours**

Assigned footprints to the symbols along with arranging the components on the PCB at the places i want them to be
![image.png](https://forge.hackclub.com/rails/active_storage/blobs/proxy/eyJfcmFpbHMiOnsiZGF0YSI6NzYsInB1ciI6ImJsb2JfaWQifX0=--e3be22112e2a834c446a67b5f3fb704960dfed2c/image.png)

# 2026-04-23: More work on Schematic

**Total time spent: 1.5 hour**

Completed wiring of the Display SD card and the 3 keys that i will be using for navigation on the display. Even tho the display can have its own sd card but still i will be adding a microsd card slot on the main board so that it looks clean i can use a micro Sd card instead
![image.png](https://forge.hackclub.com/rails/active_storage/blobs/proxy/eyJfcmFpbHMiOnsiZGF0YSI6NTgsInB1ciI6ImJsb2JfaWQifX0=--d57af9354584101dac15e78a00dda59e3e13499f/image.png)

# 2026-04-23: did the power part of the schematic

**Total time spent: 2 hours**

Soo i completed the power part of the circuit the board will have both 3.3v and 5v on board and wanted to add a extra thing which is the system will charge it elf and power the system simultaneously when i plug it in its called load sharing and it was my first time doing so so took me a lot of time figuring it out https://ww1.microchip.com/downloads/aemDocuments/documents/OTH/ProductDocuments/UserGuides/51746a.pdf
![image.png](https://forge.hackclub.com/rails/active_storage/blobs/proxy/eyJfcmFpbHMiOnsiZGF0YSI6NTQsInB1ciI6ImJsb2JfaWQifX0=--85b6338dc97d9fb6f68e8c01f2cfcf2684d0de95/image.png)

# 2026-04-22: Made the Symbol and Foot print for the MSP3520 disply

**Total time spent: 2 hours**

I took refernce from the data sheet https://www.lcdwiki.com/res/MSP3520/MSP3520_Size.pdf and made symbol and footprint for the display to be used 
![image.png](https://forge.hackclub.com/rails/active_storage/blobs/proxy/eyJfcmFpbHMiOnsiZGF0YSI6NTEsInB1ciI6ImJsb2JfaWQifX0=--f5897154b2e30deac236fff2bf08e96465b3934f/image.png)![image.png](https://forge.hackclub.com/rails/active_storage/blobs/proxy/eyJfcmFpbHMiOnsiZGF0YSI6NTIsInB1ciI6ImJsb2JfaWQifX0=--b616d17a47c370e6dafdee51e2c829702148ba0d/image.png)

# 2026-04-22: Did research for parts Along with started out with the schematic 

**Total time spent: 3 hours**

I started out by charting the features I would like to have on that HUB since my aim is to getting a device with which i will use and control all my other esp32 web projects 

other thing done is completing the basic circuit for ESP32 S3 and the programming IC which here is CH340C
![image.png](https://forge.hackclub.com/rails/active_storage/blobs/proxy/eyJfcmFpbHMiOnsiZGF0YSI6NTAsInB1ciI6ImJsb2JfaWQifX0=--fc7b366520796d166b7105450b7a35cab4422d70/image.png)

