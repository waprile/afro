
Afro query protocol

Afro-running Arduinos respond to the Afro query protocol. The protocol is a fixed-size packet format, designed as a compromise between human readability and efficiency in parsing. The structure of a packet is explained in the following table. Each line corresponds to one byte. Every packet is nine bytes long.

A: ASCII 65
F: ASCII 
Arduino ID
Operation
Operand 1
Operand 2, MSB
Operand 2, LSB
F
A


Afro response protocol