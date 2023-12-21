#ifndef BOOSTPYTHONCORESCANNER_H_
#define BOOSTPYTHONCORESCANNER_H_

/* 
 C++11X support Dual ABI issue 
*/
#if __GNUC__ == 5
#define _GLIBCXX_USE_CXX11_ABI 0
#endif



#include "CsIEventListenerXml.h"
#include "CsUserDefs.h"
#include "CsBarcodeTypes.h"
#include "Cslibcorescanner_xml.h"

#include <iostream>
#include <map>
#include <vector>
#include <mutex>
#include <memory>
/*
#include <boost/python.hpp>
#include <boost/python/list.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/object.hpp>
#include <boost/python/handle.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/shared_ptr.hpp>
*/
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include "pugixml.hpp"
#include <Python.h>

using namespace std;
namespace py = pybind11;

class Barcode
{
public:
	std::string code;
	int type;
};

class Scanner;

class Attribute
{
public:
	Attribute(Scanner* s) : scanner(s) {}

	py::object GetValue() { return value; }
	int SetValue(py::object v);
	int StoreValue(py::object v);

	int id;
	py::object value;
	char datatype;
	int permission;

private:
	int ExecuteSetOrStoreAttribute(CmdOpcode command, py::object v);
	void CastValueToString(py::object v, std::string &value_as_string);
	Scanner* scanner;
};

class Scanner
{
public:
	Scanner();
	virtual ~Scanner() {}

	bool active;		
	std::string type;
	std::string scannerID;
	std::string serialnumber;
	std::string GUID;
	std::string PID;
	std::string VID;
	std::string modelnumber;
	std::string DoM;
	std::string firmware;
	py::dict attributes;

	py::dict get_dict();

	void SelectImageType(std::string); // TIFF/JPG/BMP
	void SelectImageMode();
	void SelectBarcodeMode();
	void PullTrigger();
	void ReleaseTrigger();
	void FetchAttributes();
	void FetchAttributes(std::string);

	void OnBarcodeDecorator(py::object& obj);
        virtual void OnBarcode(py::object& obj);

	void OnImageDecorator(py::object& obj);
	virtual void OnImage(py::object& obj);
	std::vector<py::object> on_image;
	std::vector<py::object> on_barcode;
};

class CoreScanner : public IEventListenerXml
{
public:
	explicit CoreScanner();
	virtual ~CoreScanner();

    virtual void OnBinaryDataEvent(short eventType, int dataLength, short dataFormat, unsigned char* sfBinaryData, std::string& pScannerData);
    virtual void OnImageEvent( short eventType, int size, short imageFormat, char* sfimageData, int dataLength, std::string& pScannerData );
    virtual void OnVideoEvent( short eventType, int size, char* sfvideoData, int dataLength, std::string& pScannerData );
    virtual void OnBarcodeEvent( short eventType, std::string& pscanData );
    virtual void OnPNPEvent( short eventType, std::string ppnpData );
    virtual void OnCommandResponseEvent( short status, std::string& prspData );
    virtual void OnScannerNotification( short notificationType, std::string& pScannerData );
    virtual void OnIOEvent( short type, unsigned char data );
    virtual void OnScanRMDEvent( short eventType, std::string& prmdData );
    virtual void OnDisconnect();


	void Open();
	void FetchScanners();

	/// TODO:
    /*void GetAttribute();
    void DiscoverTunnelingDevice();
    void GetAllAttributes();
    void SetAttribute();
    void SetAttributeStore();
    void SetZeroWeight();*/
    void Close();
	void OnExit(py::object& exc_type, py::object& exc_value, py::object& tb) { Close(); }
    
	/// TODO:
    /*void RebootScanner();
    void ExecuteActionCommand(CmdOpcode opCode);
    void GetDeviceTopology();
    void FirmwareUpdate();
    void FirmwareUpdateFromPlugin();
    void StartNewFirmware();
    void AbortFirmwareUpdate();*/

	typedef py::object scanner_callback;
	void ParseScannerXML(pugi::xml_node& scanner);	
	void OnScannerAddedDecorator(py::object& obj);
	void OnScannerRemovedDecorator(py::object& obj);
	virtual void OnScannerAdded(py::object& o);
	virtual void OnScannerRemoved(py::object& o);

	std::vector<scanner_callback> on_added;
	std::vector<scanner_callback> on_removed;

	py::object get_scanners();
	std::map<std::string, py::object> _scanners;

	bool is_open;
};

#endif /* BOOSTPYTHONCORESCANNER_H_ */
