



2.0 Afro query protocol

Afro-running Arduinos respond to the Afro query protocol. The protocol is a fixed-size packet format, designed as a compromise between human readability and efficiency in parsing. The structure of a packet is explained in the following table. Each line corresponds to one byte. Every packet is nine bytes long.

A: ASCII 65
F: ASCII 70
target Arduino ID: a byte that uniquely identifies the target Arduino. FF is used for broadcast. 0 is just reserved.
Operation: a byte that indicates what the Arduino should do. An unknown operation will do nothing.
Operand 1: a byte that contains the first operand, typically the pin the operation should work on
Operand 2, MSB: mostly used as the most significant part of the second operand
Operand 2, LSB: mostly used as the least significant part of the second operand
F: ASCII 70
A: ASCII 65

2.1 Operations

in the following paragraphs o1 is the first operand (an 8 bit int) and o2 is the second operand (a 16 bit integer). The commands are assumed to operate on the target specified by the target

R: read digital pin o1
W: write digital pin o1. High if o2 is different from zero, low if otherwise.
A: analog read of analog pin o1
P: set digital pin o1 to PWM value o2. Only the LSB in o2 is significant.
T: timed pulse on pin o1 of duration o2. Maximum duration is 65535 milliseconds
Q: query Arduino: are you there?
U: control userTask: controls the running of a function called UserTask. o1 is passed as an argument, o2 controls the number of times the function is run: 0 means stop running now, 65535 (FF) means run forever, any other number means run for that number of times.
V: returns the value of power voltage +/- 10% (says the doco)

3.0 Afro response protocol

An Arduino running Afro will:

 .1 ignore all the packets that are not directed at it
 .2 respond immediately to all the packets that carry its ID
 .3 respond after a delay to broadcast packets
 
the response delay is different and fixed for each Arduino and results from hashing the Arduino ID. 

Afro response packets look like this

S:
R:
sending Arduino ID: one byte that identifies the sending Arduino. FF and 0 are reserved.
Result 1: a char value
Result 2, MSB: an int value
Result 2, LSB:
R:
S:

The precise semantics depend on the operation issued.

2.1 Responses to operations

The following examples use Z as the originating Arduino

Read (R): Result 1 contains the pin in lowercase. Result 2 is 0 if the pin is low, 1 otherwise
Write (W): there is no response.
Analog read (A): Result 1 contains the pin in UPPERCASE. Result 2 is the read value.
PWM Write (P): there is no response.
Timed pulse (T): there is no response.
Query (Q): Result 1 contains the Arduino ID. Result 2 contains the response delay to broadcast queries.
User task (U): there is no response.
