# Cold switch controller
This is the specialized polarized relay controller intended to drive an RF switches, installed inside a cryostat. It's mainly
affordable for switches similar to famous Radial products: https://www.radiall.com/products/rf-microwave-switches.html. Such switches are commonly used in experiments with superconducting RF devices, and, particularly, with quantum circuits. 

The first problem with such switches at the liquid He temperature 4K and below is that the resistance of their coils drops by ~100 times compared to room temperature values and it's no longer possible to drive them with a nominal voltage. The second is that the heat dissipation during switching must be controlled to ensure normal cryostat operation. I't especially important in case of dilution refrigerators, commonly used for experiments with quantum devices. 

To overcome the above mentioned problems the controller uses an adjustable pulsed current source instead of a voltage source to drive the coils. Both the current and pulse duration can be adjusted to achieve a reliable switching and a minimal power dissipation. Due to it's flexibility, the controller can be used to control RF switches also at room temperature which makes it a useful tool to have in you lab for test and experiment automation.

It has a minimalistic formfactor of a 3U Eurorack module with absolutely no cotrols - everything is done using a web interface or an SCPI commands.

<img src="/photos/photo_2020-07-08_19-38-45.jpg" alt="drawing" height="400"/> <img src="/photos/photo_2021-03-31_17-08-45.jpg" alt="drawing" height="400"/>

# Specs

 - Power: 12V, fully protected against overvoltage up to 30V and reversed polarity
 - Interface: Ethernet with web interface and SCPI commands
 - Output: 2 channels for polarized SPnT switches with number of positions n up to 6
 - Drive current: 10-600 mA
 - Drive pulse width: 1 - 700 ms
 - Switch coil voltage: 12 V for room temperature operation, any for cryogenic applications
