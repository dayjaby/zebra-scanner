import pprint
import time
import threading
from flask import Flask, jsonify

from zebra_scanner import CoreScanner

pp = pprint.PrettyPrinter(indent=4)
cs = CoreScanner()

app = Flask(__name__)

new_barcodes_lock = threading.RLock()
new_barcodes = {}

@cs.on_scanner_added
def on_scanner_added(scanner):
    scanner.fetch_attributes()
    scanner.pull_trigger()

    @scanner.on_barcode
    def on_barcode(barcode):
        barcodeType = "?"
        if barcode.type == 15:
            barcodeType = "CODE128"
        elif barcode.type == 6:
            barcodeType = "I25"

        new_barcodes[barcode.code] = {
            "code": barcode.code,
            "type": barcodeType
        }

@cs.on_scanner_removed
def on_scanner_removed(scanner):
    scanner.release_trigger()

@app.route("/barcodes")
def get_barcodes():
    with new_barcodes_lock:
        json = jsonify({
            "barcodes": new_barcodes.values(),
            "timestamp": int(time.time()*1000)
        })
        new_barcodes.clear()
        return json

if __name__ == '__main__':
    app.run(host="0.0.0.0", debug=True, port=8080, use_reloader=False)

