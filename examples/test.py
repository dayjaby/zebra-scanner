import sys
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
        scanner.pull_trigger()
