import pprint
import time

from zebra_scanner import CoreScanner

pp = pprint.PrettyPrinter(indent=4)
cs = CoreScanner()


@cs.on_scanner_added
def on_scanner_added(scanner):
    print("New scanner found:")
    pp.pprint(scanner.__dict__)
    scanner.pull_trigger()

    scanner.fetch_attributes()
    for id, attribute in scanner.attributes.items():
        if id<10:
            pp.pprint({
                "id": id,
                "datatype": attribute.datatype,
                "value": attribute.value,
                "permission": attribute.permission
            })

    @scanner.on_barcode
    def on_barcode(barcode):
        print("Scanned:")
        print(barcode.code, barcode.type)

@cs.on_scanner_removed
def on_scanner_removed(scanner):
    print("Scanner removed:")
    scanner.release_trigger()
    pp.pprint(scanner.__dict__)

while True:
    time.sleep(0.1)
    # do nothing while the scanner is reading in continous mode
