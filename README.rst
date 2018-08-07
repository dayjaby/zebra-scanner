**********
Installing
**********

You can easily install zebra_scanner with pip:

.. code-block:: sh

 pip install zebra_scanner


*****************
A minimal example
*****************

.. code-block:: python

 import pprint
 import time
 
 from zebra_scanner import CoreScanner
 
 pp = pprint.PrettyPrinter(indent=4)
 scanners = []
 cs = CoreScanner()
 
 
 @cs.on_scanner_added
 def on_scanner_added(scanner):
     print("New scanner found:")
     pp.pprint(scanner.__dict__)
     scanners.append(scanner)
     scanner.pull_trigger()
 
     @scanner.on_barcode
     def on_barcode(barcode):
         print("Scanned:")
         print(barcode.code, barcode.type)

 @cs.on_scanner_removed
 def on_scanner_removed(scanner):
     print("Scanner removed:")
     scanner.release_trigger()
     scanners.remove(scanner)
     pp.pprint(scanner.__dict__)
 
 while True:
     time.sleep(0.1)
     for scanner in scanners:
         # should have been done in on_scanner_added
         # but somehow it does not work immediately
         # upon registration of a new scanner
         scanner.pull_trigger()



*******************
Running the example
*******************

Creating states and state machines is pretty straightforward:

.. code-block:: sh

 ~/Development/zebra-scanner/examples$ python test.py
 New scanner found:
 {   'DoM': '10Mar18',
     'GUID': 'AFF531D4821A3E4BB2127A380DA81FB0',
     'PID': '1900',
     'VID': '05e0',
     'firwmare': 'PAABLS00-005-R05',
     'modelnumber': 'PL-3307-B100R',
     'scannerID': '1',
     'serialnumber': '00000000K10U532B',
     'type': 'SNAPI'}
 Scanned:
 ('00140092390052832143', '15')
 Scanned:
 ('01040092393881071000017500861331', '15')
 Scanned:
 ('00140092390052832143', '15')
 Scanned:
 ('00140092390052832143', '15')
 ^CScanner removed:
 {   'DoM': '10Mar18',
     'GUID': 'AFF531D4821A3E4BB2127A380DA81FB0',
     'PID': '1900',
     'VID': '05e0',
     'firwmare': 'PAABLS00-005-R05',
     'modelnumber': 'PL-3307-B100R',
     'scannerID': '1',
     'serialnumber': '00000000K10U532B',
     'type': 'SNAPI'}

