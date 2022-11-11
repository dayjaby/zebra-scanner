#include "BoostPythonCoreScanner.h"
#include <stdlib.h>
#include <sstream>
#include <boost/python/make_function.hpp>

template<class a>
py::object call_python(py::object& fn, a& arg1, bool ensure_gil=true) {
	py::object ret;
	PyGILState_STATE gstate;
	if (ensure_gil) {
		gstate = PyGILState_Ensure();
	}
	try {
		ret = fn(arg1);
	} catch(const py::error_already_set&) {
		PyObject *e, *v, *t;
		PyErr_Fetch(&e, &v, &t);
		PyObject* e_obj = PyObject_Repr(e);
		PyObject* v_obj = PyObject_Repr(v);
		std::string e_str = PyUnicode_AsUTF8(e_obj);
		std::string v_str = PyUnicode_AsUTF8(v_obj);
		cout << "Error occured: " <<  e_str << "\n";
		cout << v_str << "\n";
		PyErr_Restore(e, v, t);
		throw;
	}
	if (ensure_gil) {
		PyGILState_Release(gstate);
	}
	return ret;
}

// BOOST_PYTHON_MODULE(zebra_scanner)
PYBIND11_MODULE(zebra_scanner, m)
{
	using namespace py;

	class_<CoreScanner>(m, "CoreScanner")
		.def(py::init<>())
		.def("open", &CoreScanner::Open)
		.def("close", &CoreScanner::Close)
		.def("on_scanner_added", &CoreScanner::OnScannerAddedDecorator)
		.def("on_scanner_removed", &CoreScanner::OnScannerRemovedDecorator)
		.def("fetch_scanners", &CoreScanner::FetchScanners);

	class_<Scanner>(m, "Scanner")
		.def("on_barcode", &Scanner::OnBarcodeDecorator)
		.def("pull_trigger", &Scanner::PullTrigger)
		.def("release_trigger", &Scanner::ReleaseTrigger)
		.def("fetch_attributes", static_cast<void (Scanner::*)()>(&Scanner::FetchAttributes))
		.def("fetch_attributes", static_cast<void (Scanner::*)(std::string)>(&Scanner::FetchAttributes))
		.def_readonly("attributes", &Scanner::attributes)
		.def_readonly("type", &Scanner::type)
		.def_readonly("scannerID", &Scanner::scannerID)
		.def_readonly("serialnumber", &Scanner::serialnumber)
		.def_readonly("GUID", &Scanner::GUID)
		.def_readonly("PID", &Scanner::PID)
		.def_readonly("VID", &Scanner::VID)
		.def_readonly("modelnumber", &Scanner::modelnumber)
		.def_readonly("DoM", &Scanner::DoM)
		.def_readonly("firmware", &Scanner::firmware);

	class_<Attribute>(m, "Attribute")
		.def("get_value", &Attribute::GetValue)
		.def("set_value", &Attribute::SetValue)
		.def("store_value", &Attribute::StoreValue)
		.def_readonly("id", &Attribute::id)
		.def_readonly("permission", &Attribute::permission)
		.def_readonly("datatype", &Attribute::datatype)
		.def_readonly("value", &Attribute::value);

	// used to be class_<Barcode, std::auto_ptr<Barcode> >(m, "Barcode")
	class_<Barcode>(m, "Barcode")
		.def_readonly("code", &Barcode::code)
		.def_readonly("type", &Barcode::type);
}

Scanner::Scanner() {
}

void Scanner::OnBarcodeDecorator(py::object& obj) {
	on_barcode.push_back(obj);
}

void Scanner::OnBarcode(py::object& obj) {
	for(std::vector<py::object>::iterator i=on_barcode.begin();i!=on_barcode.end();++i) {
	    call_python(*i, obj, false);
	}
}

void Scanner::PullTrigger()
{
    StatusID status; 
    std::string inXml = "<inArgs><scannerID>" + scannerID + "</scannerID></inArgs>";
    std::string outXml;
    ::ExecCommand(CMD_DEVICE_PULL_TRIGGER, inXml, outXml, &status);
}

void Scanner::ReleaseTrigger()
{
    StatusID status;
    std::string inXml = "<inArgs><scannerID>" + scannerID + "</scannerID></inArgs>";
    std::string outXml;
    ::ExecCommand(CMD_DEVICE_RELEASE_TRIGGER, inXml, outXml, &status);
}

py::dict Scanner::get_dict() {
	py::dict d;
	d["type"] = type;
	d["scannerID"] = scannerID;
	d["serialnumber"] = serialnumber;
	d["GUID"] = GUID;
	d["PID"] = PID;
	d["VID"] = VID;
	d["modelnumber"] = modelnumber;
	d["DoM"] = DoM;
	d["firwmare"] = firmware;
	return d;
}

CoreScanner* CORE_SCANNER_SINGLETON;
#include <csignal>

void signal_handler_close(int num) {
	if(CORE_SCANNER_SINGLETON && CORE_SCANNER_SINGLETON->is_open) {
		CORE_SCANNER_SINGLETON->Close();
	}

	if(num == SIGINT || num == SIGTERM) {
		std::exit(128+num);
	}
}

CoreScanner::CoreScanner()
	: is_open(false)
{
	CORE_SCANNER_SINGLETON = this;
	PyEval_InitThreads();	
	std::signal(SIGINT, signal_handler_close);
	std::signal(SIGTERM, signal_handler_close);
	Open();
}

CoreScanner::~CoreScanner()
{
	if(is_open)
		Close();
}

void CoreScanner::Open()
{
    StatusID status;
    ::Open(this, SCANNER_TYPE_ALL, &status);

    // std::string inXml = "<inArgs><cmdArgs><arg-int>7</arg-int><arg-int>1,2,4,8,16,32,128</arg-int></cmdArgs></inArgs>";

    std::string inXml = "<inArgs><cmdArgs><arg-int>6</arg-int><arg-int>1,2,4,8,16,32</arg-int></cmdArgs></inArgs>";
    std::string outXml;
    ::ExecCommand(CMD_REGISTER_FOR_EVENTS, inXml, outXml, &status);
	FetchScanners();
	is_open = true;
}

#include <algorithm> 
#include <cctype>
#include <locale>

static inline void ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
		return !std::isspace(ch);
	}));
}

static inline void rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
		return !std::isspace(ch);
	}).base(), s.end());
}

static inline void trim(std::string &s) {
	ltrim(s);
	rtrim(s);
}

void CoreScanner::ParseScannerXML(pugi::xml_node& scanner) {
	std::string id = scanner.child_value("scannerID");
	Scanner s;
	s.active = true;
	s.type = scanner.attribute("type").value();
	s.scannerID = id;
	s.serialnumber = scanner.child_value("serialnumber");
	s.GUID = scanner.child_value("GUID");
	s.PID = scanner.child_value("PID");
	s.VID = scanner.child_value("VID");
	s.modelnumber = scanner.child_value("modelnumber");
	s.firmware = scanner.child_value("firmware");
	trim(s.serialnumber);
	trim(s.modelnumber);
	trim(s.firmware);
	s.DoM = scanner.child_value("DoM");
	PyGILState_STATE state = PyGILState_Ensure();
	py::object o = py::cast(s);
	_scanners[id] = o;
	// o.attr("__dict__").attr("update")(s.get_dict());
	PyGILState_Release(state);

	// wait for some kind of response from the scanner
	// so that we know we can access the scanner
	// in our on_scanner_added callback
	s.FetchAttributes("0");
	OnScannerAdded(o);
}

void Scanner::FetchAttributes() {
    std::string inXml = "<inArgs><scannerID>"+scannerID+"</scannerID></inArgs>";
    StatusID sId;
    std::string outXml;

    ::ExecCommand(CMD_RSM_ATTR_GETALL, inXml, outXml, &sId);

	pugi::xml_document outargs;
	outargs.load_buffer_inplace(&outXml[0], outXml.size());
	std::string attribute_list;
	bool first = true;
	for(pugi::xml_node attr = outargs.child("outArgs").child("arg-xml").child("response").child("attrib_list").child("attribute"); 
			attr; 
			attr = attr.next_sibling("attribute")) {
		if(!first)
			attribute_list += ',';
		attribute_list += attr.child_value(); 
		first = false;
	}
	FetchAttributes(attribute_list);
}

void Scanner::FetchAttributes(std::string attribute_list) {
	std::string inXml = "<inArgs><scannerID>" + scannerID + 
						"</scannerID><cmdArgs><arg-xml><attrib_list>" + 
						attribute_list + "</attrib_list></arg-xml></cmdArgs></inArgs>";
	
    StatusID sId;
	std::string outXmlA;
	::ExecCommand(CMD_RSM_ATTR_GET, inXml, outXmlA, &sId);
	pugi::xml_document outargs;
	outargs.load_buffer_inplace(&outXmlA[0], outXmlA.size());
	std::string False("False");
	for(pugi::xml_node attr = outargs.child("outArgs").child("arg-xml").child("response").child("attrib_list").child("attribute"); 
			attr; 
			attr = attr.next_sibling("attribute")) {
		Attribute a(this);
		a.id = std::stoi(attr.child_value("id"));
		a.datatype = attr.child_value("datatype")[0];
		a.permission = std::stoi(attr.child_value("permission"));
		// Information about data types is taken from zebra-scanner-4.4.1-8/Native/CsClient/src/CsDecodeIncomming.cpp
		if (a.datatype=='F') { // Flag? A boolean
			if(False.compare(attr.child_value("value")) == 0) {
				a.value = py::cast(false);
			} else {
				a.value = py::cast(true);
			}
		}
		else if (a.datatype=='B' || a.datatype=='W' || a.datatype=='D') { //uint8_t/uint16_t/uint32_t
			a.value = py::cast(std::stoul(attr.child_value("value")));
		}
		else if (a.datatype=='I' || a.datatype=='L') { // int16_t/int32_t
			a.value = py::cast(std::stol(attr.child_value("value")));
		}
		else { // String or single character
			a.value = py::cast(attr.child_value("value"));
		}
		attributes[py::cast(a.id)] = a;
	}
}

int Attribute::SetValue(py::object v) {
	return ExecuteSetOrStoreAttribute(CMD_RSM_ATTR_SET, v);
}

int Attribute::StoreValue(py::object v) {
	return ExecuteSetOrStoreAttribute(CMD_RSM_ATTR_STORE, v);
}

int Attribute::ExecuteSetOrStoreAttribute(CmdOpcode command, py::object v)
{
	std::string value_as_string;
	CastValueToString(v, value_as_string);
    std::string inXml = "<inArgs><scannerID>" + scanner->scannerID + "</scannerID><cmdArgs><arg-xml><attrib_list><attribute>" +
		"<id>" + std::to_string(id) + "</id>" +
		"<datatype>" + datatype + "</datatype>" +
		"<value>" + value_as_string + "</value>" +
		"</attribute></attrib_list></arg-xml></cmdArgs></inArgs>";
    std::string outXml;
    StatusID status;
    ::ExecCommand(command, inXml, outXml, &status);
	if (status == STATUS_OK) {
		value = v; // Update local copy of the new value
	}
	return status;
}

void Attribute::CastValueToString(py::object v, std::string &value_as_string) {
	// Information about data types is taken from zebra-scanner-4.4.1-8/Native/CsClient/src/CsDecodeIncomming.cpp
	switch (datatype) {
		case 'F': // Boolean (F for flag?)
			value_as_string = v.cast<bool>() ? "True" : "False";
			break;

		case 'B': // uint8_t
		case 'W': // uint16_t
		case 'D': // uint32_t
			value_as_string = std::to_string(v.cast<uint32_t>());
			break;

		case 'I': // int16_t
		case 'L': // int32_t
			value_as_string = std::to_string(v.cast<int32_t>());
			break;

		default: // A string
			value_as_string = v.cast<std::string>();
			break;
	}
}

void CoreScanner::OnScannerAddedDecorator(py::object& obj) {
	on_added.push_back(CoreScanner::scanner_callback(obj));
	for(std::map<std::string, py::object>::iterator i = _scanners.begin(); i != _scanners.end(); ++i) {
		py::object& o = i->second;
		Scanner& s = o.cast<Scanner&>();
		if(s.active)
			call_python(obj, o);
	}
}

void CoreScanner::OnScannerRemovedDecorator(py::object& obj) {
	on_removed.push_back(CoreScanner::scanner_callback(obj));
}

void CoreScanner::OnScannerAdded(py::object& o) {
	for(std::vector<CoreScanner::scanner_callback>::iterator i=on_added.begin();i!=on_added.end();++i) {
		call_python(*i, o);
	}
}

void CoreScanner::OnScannerRemoved(py::object& o) {
	for(std::vector<CoreScanner::scanner_callback>::iterator i=on_removed.begin();i!=on_removed.end();++i) {
		call_python(*i, o);
	}
}

void CoreScanner::FetchScanners()
{
    unsigned short count;
    vector<unsigned int> list;
    string outXml;
    StatusID eStatus;
    ::GetScanners(&count, &list, outXml, &eStatus);
    if ( eStatus != STATUS_OK )
    {
		return;
    }
	pugi::xml_document scanners;
	scanners.load_buffer_inplace(&outXml[0], outXml.size());
	for(pugi::xml_node scanner = scanners.child("scanners").child("scanner"); scanner; scanner = scanner.next_sibling("scanner")) {
		ParseScannerXML(scanner);
	}
}

void CoreScanner::Close()
{
	for(std::map<std::string, py::object>::iterator i = _scanners.begin(); i != _scanners.end(); ++i) {
		py::object& o = i->second;
		Scanner& s = o.cast<Scanner&>();
		if(s.active)
			OnScannerRemoved(o);
	}
    StatusID status;
    ::Close(0, &status);
	is_open = false;
}


/// TODO: implement python callbacks
void CoreScanner::OnBinaryDataEvent(short eventType, int dataLength, short dataFormat, unsigned char* sfBinaryData, std::string& pScannerData)
{
}

void CoreScanner::OnImageEvent( short eventType, int size, short imageFormat, char* sfimageData, int dataLength, std::string& pScannerData )
{
}

void CoreScanner::OnVideoEvent( short eventType, int size, char* sfvideoData, int dataLength, std::string& pScannerData )
{
}

void CoreScanner::OnPNPEvent( short eventType, std::string ppnpData )
{
    if (eventType == SCANNER_ATTACHED || eventType == SCANNER_DETACHED) {
		pugi::xml_document outargs;
		outargs.load_buffer_inplace(&ppnpData[0], ppnpData.size());
		for(pugi::xml_node scanner = outargs.child("outargs").child("arg-xml").child("scanners").child("scanner"); 
				scanner; 
				scanner = scanner.next_sibling("scanner")) {
			if (eventType == SCANNER_ATTACHED) {
				FetchScanners();
			} else {
				py::object& o = _scanners[scanner.child_value("scannerID")];
				Scanner& s = o.cast<Scanner&>();
				if(s.active) {
					s.active = false;
					OnScannerRemoved(o);
				}
			}
		}
    }
}

void CoreScanner::OnCommandResponseEvent( short status, std::string& prspData )
{
}

void CoreScanner::OnScannerNotification( short notificationType, std::string& pScannerData )
{
}

void CoreScanner::OnIOEvent( short type, unsigned char data )
{
}

void CoreScanner::OnScanRMDEvent( short eventType, std::string& prmdData )
{
}

void CoreScanner::OnDisconnect()
{
}

void CoreScanner::OnBarcodeEvent(short int eventType, std::string & pscanData)
{
	pugi::xml_document outargs;
	outargs.load_buffer_inplace(&pscanData[0], pscanData.size());
	pugi::xml_node args = outargs.child("outArgs");
	std::string scannerID = args.child_value("scannerID");
	std::map<std::string, py::object>::iterator si = _scanners.find(scannerID);
	if(si == _scanners.end()) {
		FetchScanners();
		return;
	}
	Scanner& s = si->second.cast<Scanner&>();
	if(!s.active) {
		FetchScanners();
		return;
	}

	for(pugi::xml_node scanData = args.child("arg-xml").child("scandata"); 
			scanData; 
			scanData = scanData.next_sibling("scandata")) {

		std::string data = scanData.child_value("datalabel");
		std::string result;
		for(std::string::iterator i=data.begin(); i!=data.end(); ++i) {
			std::stringstream ss;
			char c1 = *(i++) - 48;
			if(c1 > 9) c1 -= 39; 
			char c2 = *(i++) - 48;
			if(c2 > 9) c2 -= 39; 
			char d = 16*c1+c2;;
			result.append(1, d);
		}
		Barcode b;
		b.code = result;
		b.type = std::stoi(scanData.child_value("datatype"));
		PyGILState_STATE state = PyGILState_Ensure();
		py::object o = py::cast(b);
		o.inc_ref();
		s.OnBarcode(o);
		PyGILState_Release(state);
	}
}

