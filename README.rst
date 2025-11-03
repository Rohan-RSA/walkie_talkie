Overview
********

LoRa based walkie talkie. The radios are constantly in receive mode unless a transmission is trigerred by pressing button 1 at this time.
Later on in the project, a button will be pressed and kept while voice (audio) is being recorded, compressed and sent. After this the radio
will return to receive mode again. CAD or Cannel Activity Detect will be introduced aswell. The bandwidth will also be divided up into channels.

The HMI will be a 4.0inch resistive touch display with a pen. The user can send text or audio as described above. The handheld radio will also
keep time, probably with a RTC and there is a possibility to add GNSS to the project in order to send a location (coordinates) to another peer.

Description continues..

Building and Running
********************

Build and flash the sample as follows, changing ``b_l072z_lrwan1`` for
your board, where your board has a ``lora0`` alias in the devicetree.

.. zephyr-app-commands::
   :zephyr-app: c:\Projects\Github\LoRa_Radio\walkie_talkie\build\nrf5340dk\nrf5340\cpuapp
   :host-os: windows 11
   :board: nrf52840
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

*** Booting Zephyr OS build v4.2.0 ***
[00:00:00.329,681] <inf> main: LoRa modem configuring succeeded
[00:00:00.329,772] <inf> main: Set up button at gpio@842500 pin 23
[00:00:00.329,864] <inf> main: Set up LED at gpio@842500 pin 29
[00:00:00.329,956] <inf> main: Radio is in receive mode. Press button 1 to send a LoRa packet
[00:00:09.629,577] <inf> main: Transmitted bytes = 12
0x43 0x68 0x65 0x76 0x79 0x20 0x4c 0x53 0x31 0x20 0x35 0x2e
RECV 12 bytes: 0x43 0x68 0x65 0x76 0x79 0x20 0x4c 0x53 0x31 0x20 0x35 0x2e RSSI = -35dBm, SNR = 13dBm

