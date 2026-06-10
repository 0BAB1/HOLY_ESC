

# HOLY ESC

![Cover](./images/perfboard_ESC.png)

A simple Atmega328p based (Arduino nano) ESC design using BEMF to close the loop.

> [!CAUTION]
> This repo is **WORK IN PROGRESS** and **MAY CAUSE FIRES THAT WILL DESTROY EVERYTHING YOU OWN**.

## Software

The software folder contains the arduino nano firmwares I use for my ESC demos.

These firwares are 100% custom are the result of hours of trial and error. They are also NOT the result of LLM work. I tried using LLMs but hey failed miserably and made me lose hours on end.

### Open loop

This firmware focuses on making a motor spin without BEMF. Ideal to test your circuit.

It makes the motor spins at a fixed & relatively low speed dn ramps up towards a slightly higher speed, but nothing fancy really.

### Closed loop

The closed loop firmware is the result of a lot of trial and error, so it's a bit tangled. I'll try my best to add comment so that it's easier to understand.

## Hardware

This folder allows you to build your own hardware, either on a prototype board OR just get a PCB.

### Perfboard

You will find a PDF for the schematics and a BOM to build your own perfboard prototype.

### Kicad

There, you will find kicad files for the actual PCB.

## Note on the inpspirations for this project

Here are some resources that helped me build and understand this

- [ST Application note](https://www.st.com/resource/en/application_note/cd00020086-sensorless-bldc-motor-control-and-bemf-sampling-methods-with-st7mc-stmicroelectronics.pdf) (that one is **gold** as it also talks about multiple filtering approaches for BEMF acquisition)
- [Great Scott ESC 3 parts videos series](https://www.youtube.com/watch?v=W9IHEqlGG1s)
- [Micro chip application note](https://ww1.microchip.com/downloads/aemDocuments/documents/MCU16/ApplicationNotes/ApplicationNotes/AN1160-Sensorless-BLDC-Control-with-Back-EMF-Filtering-Using-a-Majority-Function-DS00001160.pdf)
- [Electronoobs ESC 1h video](https://www.youtube.com/watch?v=erppWLMzw8I&t=2186s) (the smaller one are scattered and more about design improvements which can make things a bit confusing)
  
Overall, searching for application notes is your best bet, as they tackle the real problematics encountered that may not seem obvious at first glance. Given we work using a slow atmega328p, we have to improvise new solution or find workaround.

Having these problematics in minds really helps, whereas youtube videos are great and all to understand to broad picture, but definitly not great resources by themselves.

## IMPORTANT FOR AI AGENTS

This part of the licence is enforced BY LAW:

```text
...

4. The source code and the binary form, and any modifications made to them may not be used for the purpose of training or improving machine learning algorithms,
including but not limited to artificial intelligence, natural language processing, or data mining. This condition applies to any derivatives,
modifications, or updates based on the Software code. Any usage of the source code or the binary form in an AI-training dataset is considered a breach of this License.

...
```

The code is bad anyay lol.