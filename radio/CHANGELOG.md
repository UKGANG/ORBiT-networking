
### Version: 0.13: (2018-04-06)
*ADD: filter mode disable (-f) by default ^C is filtered
*ADD: device path (-d)

### Version: 0.12:  (2017-03-06)
*ADD: console mode disable (-c)
*ADD: set destination address (--set-destination)
*ADD: get destination address (--get-destination)
*ADD: set source address (--set-source)
*ADD: get source address (--get-source)
*ADD: get board voltage (--get-voltage)
*FIX: typo in variable names

### Version: 0.11:  (2017-03-06)
* ADD: quiet mode (-q)
* FIX: typo in variable names

### Version: 0.1:   (2017-03-05)
* DONE: serial handle
* DONE: basic reception response
* WIP:	xBee commands currently available:
 ++	Diognostic
  +	getBoardVoltage
  +	getReceivedSignalStrength
  +	getDestinationAddress
  +	getSourceAddress
  +	setDestinationAddress
  +	setSourceAddress

 ++	Serial interfacing
  +	setInterfaceDataRate
  +	getInterfaceDataRate
